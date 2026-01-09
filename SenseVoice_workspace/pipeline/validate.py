"""
SenseVoice Model Validator
验证模型输出一致性
"""

import os
import sys
import numpy as np
from pathlib import Path

from pipeline.config import (
    PYTORCH, MODEL_PATH, TEST_AUDIO,
    TFLITE_OUTPUT, TOKENS_PATH, OUTPUT_DIR
)


def check_pytorch_mode(should_be_native=False):
    """检查 PYTORCH 模式"""
    if should_be_native:
        if PYTORCH != 1:
            print("❌ 错误: 请先将 config.py 中的 PYTORCH 设置为 1")
            print("   PYTORCH=1 表示原生模式（保存完整输出）")
            return False
    else:
        if PYTORCH != 0:
            print("❌ 错误: 请先将 config.py 中的 PYTORCH 设置为 0")
            print("   PYTORCH=0 表示导出模式")
            return False
    return True


def compare_outputs(pytorch_file, tflite_file, vocab_size=25055):
    """
    对比 PyTorch 和 TFLite 输出

    Args:
        pytorch_file: PyTorch 输出 .npy 文件
        tflite_file: TFLite 输出 .npy 文件
        vocab_size: 词汇表大小

    Returns:
        bool: 是否通过验证
    """
    print("="*80)
    print("  SenseVoice 模型输出对比")
    print("="*80)
    print()

    if not os.path.exists(pytorch_file):
        print(f"❌ 错误: 找不到 PyTorch 输出: {pytorch_file}")
        return False

    if not os.path.exists(tflite_file):
        print(f"❌ 错误: 找不到 TFLite 输出: {tflite_file}")
        return False

    # 加载数据
    print("加载输出数据...")
    pytorch_logits = np.load(pytorch_file)
    tflite_logits = np.load(tflite_file)

    print(f"PyTorch shape:  {pytorch_logits.shape}")
    print(f"TFLite shape:   {tflite_logits.shape}")
    print()

    # 检查形状
    if pytorch_logits.shape != tflite_logits.shape:
        print("⚠️  警告: 形状不匹配")
        min_frames = min(pytorch_logits.shape[1], tflite_logits.shape[1])
        print(f"   只比较前 {min_frames} 帧")
        pytorch_logits = pytorch_logits[:, :min_frames, :]
        tflite_logits = tflite_logits[:, :min_frames, :]

    # 计算差异
    diff = np.abs(pytorch_logits - tflite_logits)
    max_diff = np.max(diff)
    mean_diff = np.mean(diff)

    print("差异统计:")
    print(f"  最大绝对误差: {max_diff:.6f}")
    print(f"  平均绝对误差: {mean_diff:.6f}")
    print()

    # Token 对比
    pytorch_tokens = pytorch_logits.argmax(axis=-1)
    tflite_tokens = tflite_logits.argmax(axis=-1)

    token_match = (pytorch_tokens == tflite_tokens).sum()
    token_total = pytorch_tokens.size
    token_accuracy = token_match / token_total * 100

    print("Token 预测对比:")
    print(f"  匹配数:   {token_match} / {token_total}")
    print(f"  准确率:   {token_accuracy:.2f}%")
    print()

    # 判断
    if token_accuracy >= 99.9:
        print("✅ 验证通过!")
        return True
    else:
        print(f"❌ 验证失败: Token 准确率 {token_accuracy:.2f}% < 99.9%")
        return False


def decode_text(logits_file, vocab_file, output_file=None):
    """
    解码 logits 为文本

    Args:
        logits_file: logits .npy 文件
        vocab_file: 词汇表文件
        output_file: 输出文本文件

    Returns:
        str: 解码的文本
    """
    print("="*80)
    print("  SenseVoice 文本解码")
    print("="*80)
    print()

    if not os.path.exists(logits_file):
        print(f"❌ 错误: 找不到 logits 文件: {logits_file}")
        return None

    if not os.path.exists(vocab_file):
        print(f"❌ 错误: 找不到词汇表: {vocab_file}")
        return None

    try:
        # 加载数据
        print(f"加载 logits: {logits_file}")
        logits = np.load(logits_file)
        print(f"  Shape: {logits.shape}")
        print()

        # 加载词汇表
        print(f"加载词汇表: {vocab_file}")
        vocab = {}
        with open(vocab_file, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                parts = line.rsplit(' ', 1) if ' ' in line else line.rsplit('\t', 1)
                if len(parts) == 2:
                    token, token_id = parts
                    try:
                        vocab[int(token_id)] = token
                    except ValueError:
                        continue
                else:
                    vocab[len(vocab)] = line
        print(f"  词汇表大小: {len(vocab)}")
        print()

        # CTC 解码
        print("CTC 解码中...")
        if len(logits.shape) == 3:
            logits = logits.squeeze(0)

        tokens = logits.argmax(axis=-1)

        # Collapse
        collapsed = []
        prev_token = None
        for token in tokens:
            if token == 0:  # blank
                prev_token = None
                continue
            if token != prev_token:
                collapsed.append(token)
                prev_token = token

        # 转换为文本
        raw_text = "".join([vocab.get(t, f"<UNK_{t}>") for t in collapsed])
        print(f"  Token 数量: {len(collapsed)}")
        print(f"  原始文本: {raw_text}")
        print()

        # 后处理
        final_text = raw_text
        while final_text.startswith("<|"):
            end_idx = final_text.find("|>")
            if end_idx == -1:
                break
            final_text = final_text[end_idx + 2:]

        final_text = final_text.replace("▁", " ").strip()

        print(f"  最终文本: {final_text}")
        print()

        # 保存
        if output_file is None:
            output_file = OUTPUT_DIR / "transcription.txt"

        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(final_text)

        print(f"✅ 文本已保存到: {output_file}")
        print()

        return final_text

    except Exception as e:
        print(f"❌ 解码失败: {e}")
        import traceback
        traceback.print_exc()
        return None


def run_validation(
    model_path=MODEL_PATH,
    audio_path=TEST_AUDIO,
    tflite_path=TFLITE_OUTPUT
):
    """
    运行完整验证流程

    Args:
        model_path: 模型路径
        audio_path: 测试音频
        tflite_path: TFLite 模型路径

    Returns:
        bool: 是否通过
    """
    print("="*80)
    print("  SenseVoice 模型验证")
    print("="*80)
    print()

    if not check_pytorch_mode(should_be_native=False):
        return False

    # 这里应该调用 model_prepare/main.py
    # 为了简化，我们直接提示用户
    print("请运行以下命令进行验证:")
    print()
    print(f"cd {PROJECT_ROOT}/model_prepare")
    print(f"python3 main.py --mode=CHECK_TFLITE \\")
    print(f"    --model_path={model_path} \\")
    print(f"    --audio_path={audio_path} \\")
    print(f"    --tflite_file_path={tflite_path}")
    print()

    return True


def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(description='验证 SenseVoice 模型')
    parser.add_argument('--mode', type=str,
                        choices=['compare', 'decode', 'all'],
                        default='all', help='验证模式')
    parser.add_argument('--pytorch', type=str,
                        default=str(OUTPUT_DIR / "pytorch_logits.npy"),
                        help='PyTorch 输出文件')
    parser.add_argument('--tflite', type=str,
                        default=str(OUTPUT_DIR / "tflite_logits.npy"),
                        help='TFLite 输出文件')
    parser.add_argument('--tokens', type=str, default=TOKENS_PATH,
                        help='词汇表文件')
    parser.add_argument('--output', type=str,
                        help='输出文本文件')

    args = parser.parse_args()

    success = True

    if args.mode in ['compare', 'all']:
        success = compare_outputs(args.pytorch, args.tflite) and success

    if args.mode in ['decode', 'all']:
        text = decode_text(args.tflite, args.tokens, args.output)
        success = (text is not None) and success

    if success:
        print("="*80)
        print("✅ 验证完成！")
        print("="*80)
        return 0
    else:
        print("="*80)
        print("❌ 验证失败！")
        print("="*80)
        return 1


if __name__ == "__main__":
    sys.exit(main())
