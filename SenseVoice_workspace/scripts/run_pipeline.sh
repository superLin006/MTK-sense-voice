#!/bin/bash
#
# SenseVoice 完整流程脚本
# 一键完成：导出 → 转换 → 编译
#

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 项目根目录
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  SenseVoice Pipeline${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 解析参数
STEPS="all"

while [[ $# -gt 0 ]]; do
    case $1 in
        --steps)
            STEPS="$2"
            shift 2
            ;;
        --help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --steps STEPS     要执行的步骤 (默认: all)"
            echo "                    可选: export, compile, all"
            echo "  --help            显示帮助"
            echo ""
            echo "示例:"
            echo "  $0                  # 执行所有步骤"
            echo "  $0 --steps export  # 只导出 (PT + TFLite)"
            echo "  $0 --steps compile # 只编译 DLA"
            echo ""
            echo "步骤说明:"
            echo "  export  - 导出 PyTorch → TorchScript (.pt)"
            echo "           导出 PyTorch → TFLite (.tflite)"
            echo "  compile - 编译 TFLite → DLA (.dla)"
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            echo "使用 --help 查看帮助"
            exit 1
            ;;
    esac
done

# 步骤1: 导出模型 (PT + TFLite)
if [[ "$STEPS" == "all" || "$STEPS" == "export" ]]; then
    echo -e "${YELLOW}[步骤 1/2] 导出模型${NC}"
    echo ""
    python3 -m pipeline.export --format all
    echo ""
    echo -e "${GREEN}✓ 导出完成${NC}"
    echo ""
fi

# 步骤2: 编译 DLA
if [[ "$STEPS" == "all" || "$STEPS" == "compile" ]]; then
    echo -e "${YELLOW}[步骤 2/2] 编译 DLA${NC}"
    echo ""
    python3 -m pipeline.convert
    echo ""
    echo -e "${GREEN}✓ 编译完成${NC}"
    echo ""
fi

# 总结
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  完成！${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "生成的文件:"
echo "  - model_prepare/model/sensevoice_complete.pt"
echo "  - model_prepare/model/sensevoice_complete.tflite"
echo "  - compile/sensevoice_MT8371.dla"
echo ""
echo -e "${GREEN}✓ 所有步骤完成！${NC}"
echo ""
