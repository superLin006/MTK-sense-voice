#!/bin/bash
#
# 测试重构后的代码结构
#

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  测试重构后的结构${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# 测试1: 检查目录结构
echo -e "${YELLOW}[测试 1/6] 检查目录结构${NC}"
echo ""

required_dirs=("pipeline" "scripts" "docs")
all_exist=true

for dir in "${required_dirs[@]}"; do
    if [ -d "$dir" ]; then
        echo -e "  ${GREEN}✓${NC} $dir/"
    else
        echo -e "  ${RED}✗${NC} $dir/ (缺失)"
        all_exist=false
    fi
done

if [ "$all_exist" = true ]; then
    echo -e "${GREEN}✅ 目录结构检查通过${NC}"
else
    echo -e "${RED}❌ 目录结构检查失败${NC}"
    exit 1
fi
echo ""

# 测试2: 检查 Python 模块
echo -e "${YELLOW}[测试 2/6] 检查 Python 模块${NC}"
echo ""

modules=(
    "pipeline.config"
    "pipeline.export"
    "pipeline.convert"
    "pipeline.validate"
)

for module in "${modules[@]}"; do
    if python3 -c "import $module" 2>/dev/null; then
        echo -e "  ${GREEN}✓${NC} $module"
    else
        echo -e "  ${RED}✗${NC} $module (导入失败)"
    fi
done
echo ""

# 测试3: 检查脚本
echo -e "${YELLOW}[测试 3/6] 检查脚本${NC}"
echo ""

if [ -x "scripts/run_pipeline.sh" ]; then
    echo -e "  ${GREEN}✓${NC} scripts/run_pipeline.sh (可执行)"
else
    echo -e "  ${RED}✗${NC} scripts/run_pipeline.sh (不可执行)"
fi
echo ""

# 测试4: 检查文档
echo -e "${YELLOW}[测试 4/6] 检查文档${NC}"
echo ""

docs=("docs/README.md" "README.md")

for doc in "${docs[@]}"; do
    if [ -f "$doc" ]; then
        echo -e "  ${GREEN}✓${NC} $doc"
    else
        echo -e "  ${RED}✗${NC} $doc (缺失)"
    fi
done
echo ""

# 测试5: 检查备份
echo -e "${YELLOW}[测试 5/6] 检查备份${NC}"
echo ""

if [ -d "model_prepare.deprecated" ]; then
    file_count=$(find model_prepare.deprecated -type f | wc -l)
    echo -e "  ${GREEN}✓${NC} model_prepare.deprecated/ ($file_count 个文件)"
else
    echo -e "  ${YELLOW}⚠${NC}  model_prepare.deprecated/ (不存在)"
fi
echo ""

# 测试6: 模块功能测试（帮助信息）
echo -e "${YELLOW}[测试 6/6] 模块功能测试${NC}"
echo ""

echo "测试 pipeline.export --help"
if python3 -m pipeline.export --help &>/dev/null; then
    echo -e "  ${GREEN}✓${NC} pipeline.export --help"
else
    echo -e "  ${RED}✗${NC} pipeline.export --help"
fi

echo "测试 pipeline.convert --help"
if python3 -m pipeline.convert --help &>/dev/null; then
    echo -e "  ${GREEN}✓${NC} pipeline.convert --help"
else
    echo -e "  ${RED}✗${NC} pipeline.convert --help"
fi

echo "测试 scripts/run_pipeline.sh --help"
if bash scripts/run_pipeline.sh --help &>/dev/null; then
    echo -e "  ${GREEN}✓${NC} scripts/run_pipeline.sh --help"
else
    echo -e "  ${RED}✗${NC} scripts/run_pipeline.sh --help"
fi
echo ""

# 总结
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  测试总结${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "${GREEN}✅ 基本结构检查完成！${NC}"
echo ""
echo "下一步:"
echo "  1. 如果所有测试通过，可以清理不需要的文件"
echo "  2. 提交到 Git"
echo ""
