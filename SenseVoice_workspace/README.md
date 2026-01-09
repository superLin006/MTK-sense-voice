# SenseVoice Workspace

SenseVoice Small 模型的导出、转换和验证工作区。

## 🚀 快速开始

```bash
# 一键完成所有步骤
bash scripts/run_pipeline.sh
```

这将自动执行：
1. 导出 PyTorch → TorchScript
2. 转换 TorchScript → TFLite
3. 编译 TFLite → DLA (MT8371)
4. 验证模型一致性

## 📂 项目结构

```
SenseVoice_workspace/
├── pipeline/              # 核心流程模块 ⭐
│   ├── config.py         # 统一配置
│   ├── export.py         # 模型导出
│   ├── convert.py        # 格式转换
│   └── validate.py       # 验证工具
│
├── scripts/              # 自动化脚本
│   └── run_pipeline.sh  # 一键执行 ⭐
│
├── docs/                 # 文档
│   └── README.md        # 详细使用说明
│
├── compile/              # DLA 编译
│   └── compile_sensevoice_fp.sh
│
├── model_prepare/        # 原有文件（已废弃）
│   ├── main.py
│   ├── pt2tflite.py
│   └── ...
│
├── models/               # 模型文件
│   └── sensevoice-small/
│
└── audios/               # 测试音频
    └── test_en.wav
```

## 📖 使用方式

### 方式1: 一键脚本（推荐）

```bash
# 执行所有步骤
bash scripts/run_pipeline.sh

# 只执行特定步骤
bash scripts/run_pipeline.sh --steps export
bash scripts/run_pipeline.sh --steps convert
bash scripts/run_pipeline.sh --steps validate

# 跳过 DLA 编译
bash scripts/run_pipeline.sh --skip-compile
```

### 方式2: Python 模块

```bash
# 导出
python3 -m pipeline.export

# 转换
python3 -m pipeline.convert --mode tflite
python3 -m pipeline.convert --mode dla

# 验证
python3 -m pipeline.validate --mode compare
```

## ⚙️ 配置

编辑 `pipeline/config.py` 来修改配置：

```python
# PYTORCH 模式: 0=导出模式, 1=原生模式
PYTORCH = 0

# 路径配置
MODEL_PATH = "models/sensevoice-small"
TEST_AUDIO = "audios/test_en.wav"
```

## 📚 详细文档

查看 [`docs/README.md`](docs/README.md) 获取完整的使用说明。

## 🔗 相关项目

- **C++ 实现**: `../sensevoice_mtk_cpp/` - Android NPU 推理代码
- **GitHub**: https://github.com/superLin006/MTK-sense-voice

## ⚠️ 重要说明

### 已废弃的文件

`model_prepare/` 目录下的文件已废弃，请使用 `pipeline/` 模块替代：

| 旧文件 | 新模块 |
|--------|--------|
| `1_save_pt.sh` | `python3 -m pipeline.export` |
| `2_pt2tflite.sh` | `python3 -m pipeline.convert --mode tflite` |
| `3_check_tflite.sh` | `python3 -m pipeline.validate --mode compare` |

### 迁移指南

如果您之前使用旧的脚本，现在应该：

**旧方式：**
```bash
cd model_prepare
bash 1_save_pt.sh
bash 2_pt2tflite.sh
bash 3_check_tflite.sh
```

**新方式：**
```bash
bash scripts/run_pipeline.sh
```

---

## 📝 更新日志

- **2025-01-09**: 重构为 pipeline 结构，简化使用流程
  - 创建 `pipeline/` 核心模块
  - 添加一键执行脚本 `scripts/run_pipeline.sh`
  - 废弃 `model_prepare/` 中的旧脚本
  - 统一配置管理

---

## 🙋 获取帮助

```bash
# 查看脚本帮助
bash scripts/run_pipeline.sh --help

# 查看 Python 模块帮助
python3 -m pipeline.export --help
python3 -m pipeline.convert --help
python3 -m pipeline.validate --help
```
