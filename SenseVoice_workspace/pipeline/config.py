"""
SenseVoice Pipeline Configuration
统一的配置管理
"""

import os
from pathlib import Path

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent
MODEL_DIR = PROJECT_ROOT / "models" / "sensevoice-small"
AUDIO_DIR = PROJECT_ROOT / "audios"
OUTPUT_DIR = PROJECT_ROOT / "output"

# 模型路径
MODEL_PATH = str(MODEL_DIR)
TOKENS_PATH = str(MODEL_DIR / "tokens.txt")
CMVN_PATH = str(MODEL_DIR / "am.mvn")

# 输出路径
PT_OUTPUT = str(PROJECT_ROOT / "model_prepare" / "model" / "sensevoice_complete.pt")
TFLITE_OUTPUT = str(PROJECT_ROOT / "model_prepare" / "model" / "sensevoice_complete.tflite")
DLA_OUTPUT = str(PROJECT_ROOT / "compile" / "sensevoice_MT8371.dla")

# 测试音频
TEST_AUDIO = str(AUDIO_DIR / "test_en.wav")

# PYTORCH 模式开关
# 0 = 导出模式 (用于 TFLite/DLA 转换)
# 1 = 原生模式 (用于 PyTorch 推理和基准测试)
PYTORCH = 0

# SenseVoice 配置
SAMPLE_RATE = 16000
N_MELS = 80
LFR_M = 7  # LFR 合并帧数
LFR_N = 6  # LFR 下采样因子
VOCAB_SIZE = 25055

# Prompt 配uration
LANGUAGE_MAP = {
    "auto": 0,
    "zh": 3,
    "en": 4,
    "yue": 7,
    "ja": 11,
    "ko": 12,
    "nospeech": 13
}

TEXT_NORM_MAP = {
    "withitn": 14,
    "woitn": 15
}

EVENT_MAP = {
    "HAPPY": 1,
    "SAD": 2,
    "ANGRY": 3,
    "NEUTRAL": 4
}

EVENT_TYPE_MAP = {
    "Speech": 2,
    "Music": 3,
    "Applause": 4
}


def get_prompt(language="auto", text_norm="woitn", event="HAPPY", event_type="Speech"):
    """
    获取 prompt 配置

    Args:
        language: 语言代码
        text_norm: 文本规范化模式
        event: 事件类型
        event_type: 事件类型

    Returns:
        list: [language_id, event_id, event_type_id, text_norm_id]
    """
    language_id = LANGUAGE_MAP.get(language, 4)
    text_norm_id = TEXT_NORM_MAP.get(text_norm, 15)
    event_id = EVENT_MAP.get(event, 1)
    event_type_id = EVENT_TYPE_MAP.get(event_type, 2)

    return [language_id, event_id, event_type_id, text_norm_id]


def ensure_output_dir():
    """确保输出目录存在"""
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    return OUTPUT_DIR
