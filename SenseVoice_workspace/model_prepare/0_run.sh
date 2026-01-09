#!/bin/bash
set -ex

# SenseVoice-Small ASR test
python3 main.py --mode="PYTORCH" \
    --model_path="../models/sensevoice-small" \
    --audio_path="../audios/test_en.wav"
