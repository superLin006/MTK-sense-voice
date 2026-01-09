"""
SenseVoice Model Exporter
导出 PyTorch 模型为 TorchScript 和 TFLite 格式
"""

import sys
import os
import subprocess
from pathlib import Path

# 添加父目录到路径
sys.path.insert(0, str(Path(__file__).parent.parent))

from pipeline.config import PYTORCH, MODEL_PATH, PROJECT_ROOT
from pipeline.config import PT_OUTPUT, TFLITE_OUTPUT, ensure_output_dir


def check_pytorch_mode():
    """检查 PYTORCH 模式是否正确"""
    if PYTORCH != 0:
        print("❌ 错误: 请先将 config.py 中的 PYTORCH 设置为 0")
        print("   PYTORCH=0 表示导出模式")
        print("   当前: PYTORCH={}".format(PYTORCH))
        return False
    return True


def export_torchscript(model_path=MODEL_PATH, output_path=PT_OUTPUT):
    """
    导出 TorchScript 模型

    Args:
        model_path: SenseVoice 模型路径
        output_path: 输出 .pt 文件路径

    Returns:
        bool: 是否成功
    """
    print("="*80)
    print("  [1/2] 导出 TorchScript 模型")
    print("="*80)
    print()

    if not check_pytorch_mode():
        return False

    print("配置:")
    print(f"  模型路径: {model_path}")
    print(f"  输出路径: {output_path}")
    print(f"  PYTORCH模式: {PYTORCH} (导出模式)")
    print()

    # 导入 model_prepare 中的模块
    model_prepare_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    model_prepare_dir = os.path.join(model_prepare_dir, "model_prepare.deprecated")
    sys.path.insert(0, model_prepare_dir)

    try:
        from model_utils import create_sensevoice_model, save_torchscript

        print("[1/3] 加载模型...")
        model, cfg = create_sensevoice_model(model_path)
        print("✅ 模型加载成功")
        print(f"   词汇表大小: {cfg['vocab_size']}")
        print(f"   特征维度: {cfg['feat_dim']}")
        print()

        print("[2/3] 导出为 TorchScript...")
        ensure_output_dir()
        save_torchscript(model, output_path)
        print("✅ TorchScript 导出成功")
        print()

        print("[3/3] 验证导出文件...")
        if os.path.exists(output_path):
            file_size = os.path.getsize(output_path) / (1024 * 1024)
            print(f"✅ 文件已生成: {output_path}")
            print(f"   文件大小: {file_size:.2f} MB")
            print()
            return True
        else:
            print("❌ 文件未生成")
            return False

    except Exception as e:
        print(f"❌ 导出失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def export_tflite(pt_path=PT_OUTPUT, tflite_path=TFLITE_OUTPUT, float_mode=True):
    """
    导出 TFLite 模型

    Args:
        pt_path: 输入 .pt 文件路径
        tflite_path: 输出 .tflite 文件路径
        float_mode: 是否使用 float32 (否则使用 int8)

    Returns:
        bool: 是否成功
    """
    print("="*80)
    print("  [2/2] 导出 TFLite 模型")
    print("="*80)
    print()

    print("配置:")
    print(f"  输入: {pt_path}")
    print(f"  输出: {tflite_path}")
    print(f"  浮点模式: {float_mode}")
    print()

    if not os.path.exists(pt_path):
        print(f"❌ 错误: 找不到输入文件 {pt_path}")
        print("   请先导出 TorchScript 模型")
        return False

    try:
        print("[1/2] 调用 pt2tflite.py...")

        # 构建命令
        model_prepare_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "model_prepare.deprecated")
        pt2tflite_script = os.path.join(model_prepare_dir, "pt2tflite.py")

        cmd = [
            "python3",
            pt2tflite_script,
            "-i", pt_path,
            "-o", tflite_path
        ]

        if float_mode:
            cmd.extend(["--float", "1"])
        else:
            cmd.extend(["--float", "0"])

        print(f"命令: {' '.join(cmd)}")
        print()

        # 运行转换
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print("❌ 转换失败")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            return False

        print(result.stdout)
        print("✅ TFLite 转换成功")
        print()

        print("[2/2] 验证输出文件...")
        if os.path.exists(tflite_path):
            file_size = os.path.getsize(tflite_path) / (1024 * 1024)
            print(f"✅ 文件已生成: {tflite_path}")
            print(f"   文件大小: {file_size:.2f} MB")
            print()
            return True
        else:
            print("❌ 文件未生成")
            return False

    except Exception as e:
        print(f"❌ 转换失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def export_all(model_path=MODEL_PATH):
    """
    导出所有格式（TorchScript + TFLite）

    Args:
        model_path: 模型路径

    Returns:
        bool: 是否全部成功
    """
    print()
    print("="*80)
    print("  SenseVoice 模型导出")
    print("="*80)
    print()

    success = True

    # 导出 TorchScript
    success = export_torchscript(model_path) and success

    # 导出 TFLite
    success = export_tflite() and success

    return success


def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(description='导出 SenseVoice 模型')
    parser.add_argument('--format', type=str, choices=['pt', 'tflite', 'all'],
                        default='all', help='导出格式')
    parser.add_argument('--model_path', type=str, default=MODEL_PATH,
                        help='模型路径')
    parser.add_argument('--pt_output', type=str, default=PT_OUTPUT,
                        help='TorchScript 输出路径')
    parser.add_argument('--tflite_output', type=str, default=TFLITE_OUTPUT,
                        help='TFLite 输出路径')
    parser.add_argument('--float', type=int, default=1,
                        help='是否使用 float32 (1) 或 int8 (0)')

    args = parser.parse_args()

    success = True

    if args.format in ['pt', 'all']:
        success = export_torchscript(args.model_path, args.pt_output) and success

    if args.format in ['tflite', 'all']:
        success = export_tflite(args.pt_output, args.tflite_output, args.float == 1) and success

    if success:
        print("="*80)
        print("✅ 导出完成！")
        print("="*80)
        print()
        print("生成的文件:")
        if args.format in ['pt', 'all']:
            print(f"  - {args.pt_output}")
        if args.format in ['tflite', 'all']:
            print(f"  - {args.tflite_output}")
        print()
        return 0
    else:
        print("="*80)
        print("❌ 导出失败！")
        print("="*80)
        return 1


if __name__ == "__main__":
    sys.exit(main())
