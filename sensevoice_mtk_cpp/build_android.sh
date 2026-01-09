#!/bin/bash

# SenseVoice Android 构建脚本
# 仅编译音频处理测试程序（不包含 DLA 推理）

set -e  # 遇到错误立即退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  SenseVoice Android Build Script"
echo "========================================"
echo ""

# 配置
NDK_PATH="/home/xh/Android/Ndk/android-ndk-r25c"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build_android"
OUTPUT_DIR="$PROJECT_DIR/install/android/arm64-v8a"

# 检查 NDK
if [ ! -d "$NDK_PATH" ]; then
    echo -e "${RED}❌ NDK not found at: $NDK_PATH${NC}"
    echo "Please install Android NDK or update NDK_PATH in this script"
    exit 1
fi

echo -e "${GREEN}✓ NDK found: $NDK_PATH${NC}"
echo ""

# 创建输出目录
mkdir -p "$BUILD_DIR"
mkdir -p "$OUTPUT_DIR/bin"

# 构建
echo "Building for arm64-v8a..."
cd "$PROJECT_DIR"

"$NDK_PATH/ndk-build" \
    NDK_PROJECT_PATH="$PROJECT_DIR" \
    NDK_LIBS_OUT="$BUILD_DIR/lib" \
    APP_BUILD_SCRIPT="$PROJECT_DIR/Android.mk" \
    NDK_APPLICATION_MK="$PROJECT_DIR/Application.mk" \
    -j$(nproc)

echo ""
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build completed successfully!${NC}"
else
    echo -e "${RED}❌ Build failed!${NC}"
    exit 1
fi

# 复制可执行文件
cp "$BUILD_DIR/lib/arm64-v8a/sensevoice_audio_test" "$OUTPUT_DIR/bin/"
cp "$BUILD_DIR/lib/arm64-v8a/libc++_shared.so" "$OUTPUT_DIR/bin/"

echo ""
echo "========================================"
echo "  Build Summary"
echo "========================================"
echo "Output: $OUTPUT_DIR/bin/sensevoice_audio_test"
echo ""
echo "To install on device:"
echo "  adb push $OUTPUT_DIR/bin/sensevoice_audio_test /data/local/tmp/"
echo "  adb shell chmod 755 /data/local/tmp/sensevoice_audio_test"
echo ""
echo -e "${GREEN}✓ Ready to test on Android device!${NC}"
echo ""
