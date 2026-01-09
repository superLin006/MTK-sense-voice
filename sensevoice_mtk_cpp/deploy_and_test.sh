#!/bin/bash

# SenseVoice Android 测试脚本
# 构建、推送到设备并运行测试

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/install/android/arm64-v8a"
AUDIO_TEST_BIN="$OUTPUT_DIR/bin/sensevoice_audio_test"
DEVICE_PATH="/data/local/tmp/sensevoice_test"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  SenseVoice Android Test Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 检查是否连接了设备
echo "Checking for connected Android devices..."
DEVICES=$(adb devices | grep -v "List" | grep "device" | wc -l)
if [ "$DEVICES" -eq 0 ]; then
    echo -e "${RED}❌ No Android device found${NC}"
    echo "Please connect an Android device and enable USB debugging"
    exit 1
fi
echo -e "${GREEN}✓ Found $DEVICES device(s)${NC}"
echo ""

# 构建
echo -e "${YELLOW}Step 1: Building for Android...${NC}"
./build_android.sh
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Build completed${NC}"
echo ""

# 准备测试音频（使用项目中的测试音频）
TEST_AUDIO="/home/xh/projects/MTK/sense-voice/SenseVoice_workspace/audios/test_en.wav"
if [ ! -f "$TEST_AUDIO" ]; then
    echo -e "${RED}❌ Test audio not found: $TEST_AUDIO${NC}"
    exit 1
fi

# 推送文件到设备
echo -e "${YELLOW}Step 2: Pushing files to device...${NC}"

# 创建设备上的目录
adb shell "mkdir -p $DEVICE_PATH"

# 推送 C++ 运行时库
echo "  Pushing libc++_shared.so..."
adb push "$OUTPUT_DIR/bin/libc++_shared.so" "$DEVICE_PATH/" > /dev/null

# 推送可执行文件
echo "  Pushing binary..."
adb push "$AUDIO_TEST_BIN" "$DEVICE_PATH/sensevoice_audio_test" > /dev/null
adb shell "chmod 755 $DEVICE_PATH/sensevoice_audio_test"

# 推送测试音频
echo "  Pushing test audio..."
adb push "$TEST_AUDIO" "$DEVICE_PATH/test_en.wav" > /dev/null

echo -e "${GREEN}✓ Files pushed successfully${NC}"
echo ""

# 运行测试
echo -e "${YELLOW}Step 3: Running audio processing test...${NC}"
echo ""
echo -e "${BLUE}========================================${NC}"

adb shell "cd $DEVICE_PATH && LD_LIBRARY_PATH=. ./sensevoice_audio_test test_en.wav"

echo ""
echo -e "${BLUE}========================================${NC}"
echo ""

# 检查退出代码
EXIT_CODE=$(adb shell "echo \$?")
if [ "$EXIT_CODE" == "0" ]; then
    echo -e "${GREEN}✓ Test completed successfully!${NC}"
    echo ""
    echo "Audio processing is working correctly on Android device!"
else
    echo -e "${RED}❌ Test failed with exit code: $EXIT_CODE${NC}"
    echo ""
    echo "Check the output above for errors"
fi

echo ""
echo "To run manually on device:"
echo "  adb shell"
echo "  cd $DEVICE_PATH"
echo "  ./sensevoice_audio_test test_en.wav"
echo ""
