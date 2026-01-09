# SenseVoice Pipeline 使用指南

## 📖 概述

本项目提供了 SenseVoice Small 模型的完整导出、转换和验证流程，用于部署到 MTK MT8371 NPU。

---

## 🚀 快速开始

### 一键运行完整流程

```bash
cd SenseVoice_workspace

# 执行所有步骤
bash scripts/run_pipeline.sh
```

这将自动完成：
1. ✅ 导出 PyTorch → TorchScript
2. ✅ 转换 TorchScript → TFLite
3. ✅ 编译 TFLite → DLA (MT8371)
4. ✅ 验证模型一致性

---

## 📝 分步执行

### 方式1: 使用 Shell 脚本

```bash
# 只导出
bash scripts/run_pipeline.sh --steps export

# 只转换
bash scripts/run_pipeline.sh --steps convert

# 只验证
bash scripts/run_pipeline.sh --steps validate

# 导出 + 转换（不验证）
bash scripts/run_pipeline.sh --steps export,convert

# 转换 + 验证（跳过DLA编译）
bash scripts/run_pipeline.sh --steps convert,validate --skip-compile
```

### 方式2: 使用 Python 模块

```bash
# 导出 TorchScript
python3 -m pipeline.export

# 转换为 TFLite
python3 -m pipeline.convert --mode tflite

# 编译为 DLA
python3 -m pipeline.convert --mode dla

# 验证模型
python3 -m pipeline.validate --mode compare
```

---

## 📂 目录结构

```
SenseVoice_workspace/
├── pipeline/              # 核心流程模块
│   ├── config.py         # 统一配置
│   ├── export.py         # 模型导出
│   ├── convert.py        # 格式转换
│   └── validate.py       # 验证工具
│
├── scripts/              # 脚本
│   └── run_pipeline.sh  # 一键执行脚本
│
├── docs/                 # 文档
│   └── README.md        # 本文件
│
├── compile/              # DLA 编译脚本
│   └── compile_sensevoice_fp.sh
│
└── model_prepare/        # 原有文件（已废弃）
    ├── main.py
    ├── pt2tflite.py
    └── ... (不再使用)
```

---

## ⚙️ 配置说明

### pipeline/config.py

**关键配置项：**

```python
# PYTORCH 模式开关
PYTORCH = 0  # 0=导出模式, 1=原生模式

# 模型路径
MODEL_PATH = "models/sensevoice-small"
TEST_AUDIO = "audios/test_en.wav"

# 输出路径
PT_OUTPUT = "model_prepare/model/sensevoice_complete.pt"
TFLITE_OUTPUT = "model_prepare/model/sensevoice_complete.tflite"
DLA_OUTPUT = "compile/sensevoice_MT8371.dla"
```

**PYTORCH 模式说明：**

| 值 | 模式 | 用途 |
|---|------|------|
| 0 | 导出模式 | SAVE_PT, CHECK_TFLITE - 简化模型输出 |
| 1 | 原生模式 | PYTORCH - 保存完整输出用于对比 |

---

## 🔄 完整工作流程

### 1. 准备环境

```bash
# 确保已安装必要的环境
conda activate MTK-sensevoice
```

### 2. 导出模型

```bash
python3 -m pipeline.export
```

**输出：** `model_prepare/model/sensevoice_complete.pt`

### 3. 转换为 TFLite

```bash
python3 -m pipeline.convert --mode tflite
```

**输出：** `model_prepare/model/sensevoice_complete.tflite`

### 4. 编译为 DLA

```bash
python3 -m pipeline.convert --mode dla
```

**输出：** `compile/sensevoice_MT8371.dla`

### 5. 验证模型

```bash
# 步骤5a: 运行 PyTorch 基准 (设置 PYTORCH=1)
# 修改 pipeline/config.py: PYTORCH = 1
python3 -m pipeline.export  # 保存基准输出

# 步骤5b: 验证 TFLite (设置 PYTORCH=0)
# 修改 pipeline/config.py: PYTORCH = 0
python3 -m pipeline.validate --mode compare
```

---

## 🛠️ 高级用法

### 自定义路径

```bash
# 自定义输入/输出路径
python3 -m pipeline.export \
    --model_path /path/to/model \
    --output /path/to/output.pt

python3 -m pipeline.convert \
    --pt /path/to/model.pt \
    --tflite /path/to/model.tflite \
    --dla /path/to/model.dla
```

### 只转换 TFLite，不编译 DLA

```bash
bash scripts/run_pipeline.sh --steps convert --skip-compile
```

---

## 📊 输出文件

| 文件 | 说明 | 大小 |
|------|------|------|
| `sensevoice_complete.pt` | TorchScript 模型 | ~900 MB |
| `sensevoice_complete.tflite` | TFLite 模型 | ~890 MB |
| `sensevoice_MT8371.dla` | DLA 模型 (MT8371) | ~450 MB |

---

## ❓ 常见问题

### Q1: PYTORCH 模式设置错误

**错误信息：**
```
❌ 错误: 请先将 config.py 中的 PYTORCH 设置为 0
```

**解决方法：**
- 导出和验证时：设置 `PYTORCH = 0`
- PyTorch 基准测试时：设置 `PYTORCH = 1`

### Q2: 找不到模型文件

**错误信息：**
```
❌ 错误: 找不到输入文件 xxx
```

**解决方法：**
- 确保模型已下载到 `models/sensevoice-small/`
- 检查路径是否正确

### Q3: DLA 编译失败

**可能原因：**
- Neuron SDK 路径不正确
- 环境变量未设置

**解决方法：**
```bash
# 检查 SDK 路径
export NEURON_SDK_PATH=/path/to/neuron_sdk

# 重新编译
python3 -m pipeline.convert --mode dla --sdk $NEURON_SDK_PATH
```

---

## 🔗 相关链接

- **项目仓库**: https://github.com/superLin006/MTK-sense-voice
- **MTK SDK**: `/home/xh/projects/MTK/0_Toolkits/neuropilot-sdk-basic-8.0.10/`
- **C++ 实现**: `../sensevoice_mtk_cpp/`

---

## 📝 更新日志

- **2025-01-09**: 重构为 pipeline 结构，简化使用流程
- **2025-01-09**: 添加一键执行脚本
- **2025-01-09**: 统一配置管理

---

## 🙋 支持

如有问题，请查看：
- `../sensevoice_mtk_cpp/README.md` - C++ 部署文档
- `VALIDATION_WORKFLOW.md` - 验证流程详解（已废弃）
