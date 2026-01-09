"""
SenseVoice DLA Compiler
将 TFLite 模型编译为 MTK NPU 的 DLA 格式
"""

import os
import subprocess
import sys
from pathlib import Path

from pipeline.config import TFLITE_OUTPUT, DLA_OUTPUT, PROJECT_ROOT


def compile_to_dla(
    tflite_path=TFLITE_OUTPUT,
    dla_path=DLA_OUTPUT,
    platform="MT8371",
    neuron_sdk_path=None
):
    """
    编译 TFLite 到 DLA（直接调用 Neuron SDK）

    Args:
        tflite_path: 输入 .tflite 文件路径
        dla_path: 输出 .dla 文件路径
        platform: 目标平台 (MT8371)
        neuron_sdk_path: Neuron SDK 路径

    Returns:
        bool: 是否成功
    """
    print("="*80)
    print("  SenseVoice DLA 编译")
    print("="*80)
    print()

    # 默认 SDK 路径
    if neuron_sdk_path is None:
        neuron_sdk_path = "/home/xh/projects/MTK/0_Toolkits/neuropilot-sdk-basic-8.0.10-build20251029/neuron_sdk"

    print("配置:")
    print(f"  输入: {tflite_path}")
    print(f"  输出: {dla_path}")
    print(f"  平台: {platform}")
    print(f"  Neuron SDK: {neuron_sdk_path}")
    print()

    # 检查输入文件
    if not os.path.exists(tflite_path):
        print(f"❌ 错误: 找不到 TFLite 文件: {tflite_path}")
        print()
        print("请先运行: python3 -m pipeline.export --format tflite")
        return False

    # 检查 SDK
    if not os.path.exists(neuron_sdk_path):
        print(f"❌ 错误: 找不到 Neuron SDK: {neuron_sdk_path}")
        print()
        print("请设置正确的 SDK 路径，或使用 --sdk 参数指定")
        return False

    # 配置编译参数
    if platform == "MT8371":
        arch = "mdla5.3,edma3.6"
        l1_size = 256
    else:
        print(f"❌ 错误: 不支持的平台 {platform}")
        return False

    try:
        # 设置环境变量
        env = os.environ.copy()
        sdk_bin = os.path.join(neuron_sdk_path, "host/bin")
        sdk_lib = os.path.join(neuron_sdk_path, "host/lib")

        env["PATH"] = f"{sdk_bin}:{env.get('PATH', '')}"
        env["LD_LIBRARY_PATH"] = f"{sdk_lib}:{env.get('LD_LIBRARY_PATH', '')}"

        # 构建输出文件名
        output_filename = os.path.basename(dla_path)

        print("[1/3] 调用 Neuron SDK 编译器...")
        print(f"  架构: {arch}")
        print(f"  L1 缓存: {l1_size}KB")
        print()

        # 构建编译命令
        cmd = [
            os.path.join(sdk_bin, "ncc-tflite"),
            f"--arch={arch}",
            "-O3",
            "--relax-fp32",
            "--opt-accuracy",
            f"--l1-size={l1_size}",
            f"-d", output_filename,
            tflite_path
        ]

        print("编译命令:")
        print(f"  ncc-tflite --arch={arch} -O3 --relax-fp32 --opt-accuracy \\")
        print(f"             --l1-size={l1_size} -d {output_filename} {tflite_path}")
        print()

        # 执行编译
        result = subprocess.run(
            cmd,
            env=env,
            capture_output=True,
            text=True,
            cwd=os.path.dirname(dla_path)
        )

        if result.returncode != 0:
            print("❌ 编译失败")
            print()
            print("STDOUT:")
            print(result.stdout)
            print()
            print("STDERR:")
            print(result.stderr)
            return False

        print(result.stdout)
        print("✅ 编译成功")
        print()

        print("[2/3] 验证输出文件...")
        if not os.path.exists(dla_path):
            print("❌ 输出文件未生成")
            return False

        file_size = os.path.getsize(dla_path) / (1024 * 1024)
        print(f"✅ DLA 文件已生成: {dla_path}")
        print(f"   文件大小: {file_size:.2f} MB")
        print()

        print("[3/3] 验证文件格式...")
        # 检查文件头
        with open(dla_path, 'rb') as f:
            header = f.read(4)
            if header == b'DLAM':
                print("✅ 文件格式正确 (DLA)")
            else:
                print(f"⚠️  文件头: {header.hex()}")

        print()
        return True

    except Exception as e:
        print(f"❌ 编译失败: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    """主函数"""
    import argparse

    parser = argparse.ArgumentParser(description='编译 SenseVoice DLA 模型')
    parser.add_argument('--tflite', type=str, default=TFLITE_OUTPUT,
                        help='TFLite 输入文件')
    parser.add_argument('--dla', type=str, default=DLA_OUTPUT,
                        help='DLA 输出文件')
    parser.add_argument('--platform', type=str, default='MT8371',
                        help='目标平台 (MT8371)')
    parser.add_argument('--sdk', type=str,
                        help='Neuron SDK 路径')

    args = parser.parse_args()

    success = compile_to_dla(
        args.tflite,
        args.dla,
        args.platform,
        args.sdk
    )

    if success:
        print("="*80)
        print("✅ DLA 编译完成！")
        print("="*80)
        print()
        print("生成的文件:")
        print(f"  - {args.dla}")
        print()
        return 0
    else:
        print("="*80)
        print("❌ DLA 编译失败！")
        print("="*80)
        return 1


if __name__ == "__main__":
    sys.exit(main())
