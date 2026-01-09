#!/bin/bash

# SenseVoice 项目清理脚本
# 删除不需要的文件和目录

echo "========================================"
echo "  Cleaning up unnecessary files"
echo "========================================"
echo ""

# 备份目录（可选）
BACKUP_DIR="backup_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# 1. 删除重复的 android/ 目录
echo "Removing duplicate android/ directory..."
if [ -d "android" ]; then
    mv android "$BACKUP_DIR/"
    echo "  ✓ Moved android/ to $BACKUP_DIR/"
fi

# 2. 删除备份文件
echo "Removing backup files..."
if [ -f "src/audio_frontend.cpp.bak" ]; then
    mv src/audio_frontend.cpp.bak "$BACKUP_DIR/"
    echo "  ✓ Moved audio_frontend.cpp.bak"
fi

# 3. 删除 Linux 构建相关文件
echo "Removing Linux build files..."
if [ -f "CMakeLists.txt" ]; then
    mv CMakeLists.txt "$BACKUP_DIR/"
    echo "  ✓ Moved CMakeLists.txt"
fi

if [ -d "build" ]; then
    mv build "$BACKUP_DIR/"
    echo "  ✓ Moved build/ directory"
fi

# 4. 删除测试文件（已被替代）
echo "Removing obsolete test files..."
if [ -d "tests" ]; then
    mv tests "$BACKUP_DIR/"
    echo "  ✓ Moved tests/ directory"
fi

# 5. 删除暂时不需要的源文件
echo "Removing temporarily unused source files..."
if [ -f "src/extract_features.cpp" ]; then
    mv src/extract_features.cpp "$BACKUP_DIR/"
    echo "  ✓ Moved extract_features.cpp"
fi

if [ -f "src/run_inference.cpp" ]; then
    mv src/run_inference.cpp "$BACKUP_DIR/"
    echo "  ✓ Moved run_inference.cpp"
fi

# 6. 删除编译生成的文件
echo "Removing generated files..."
rm -rf obj
rm -rf libs
echo "  ✓ Removed obj/ and libs/"

echo ""
echo "========================================"
echo "  Cleanup Summary"
echo "========================================"
echo "Backup location: $BACKUP_DIR"
echo ""
echo "To restore files:"
echo "  mv $BACKUP_DIR/* ."
echo ""
echo "Current structure:"
echo ""
echo "sensevoice_mtk_cpp/"
echo "├── Android.mk              # Android NDK 构建配置"
echo "├── Application.mk          # Android 应用配置"
echo "├── build_android.sh        # Android 构建脚本"
echo "├── deploy_and_test.sh      # 部署和测试脚本"
echo "├── include/                # 头文件"
echo "│   ├── audio_frontend.h"
echo "│   ├── fp16_utils.h"
echo "│   ├── sensevoice_dla_adapter.h"
echo "│   └── tokenizer.h"
echo "└── src/                    # 源文件"
echo "    ├── audio_frontend_impl.cpp    # 音频前端实现"
echo "    ├── sensevoice_dla_adapter.cpp # DLA 推理适配器"
echo "    ├── tokenizer.cpp              # 分词器"
echo "    ├── test_audio_only.cpp        # 音频测试程序"
echo "    └── main.cpp                   # 主程序（完整推理）"
echo ""
echo "✓ Cleanup completed!"
echo ""
