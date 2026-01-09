# SenseVoice MTK C++ Implementation

SenseVoice Small 语音识别模型的 MTK MT8371 NPU C++ 推理实现。

## 项目结构

```
sensevoice_mtk_cpp/
├── Android.mk              # Android NDK 构建配置
├── Application.mk          # Android 应用配置
├── build_android.sh        # Android 构建脚本
├── deploy_and_test.sh      # 部署和测试脚本
├── include/                # 头文件
│   ├── audio_frontend.h           # 音频前端接口
│   ├── fp16_utils.h               # FP16/FP32 转换工具
│   ├── sensevoice_dla_adapter.h   # DLA 推理适配器
│   └── tokenizer.h                # 分词器
└── src/                    # 源文件
    ├── audio_frontend_impl.cpp    # 音频前端实现 (WAV读取 + Fbank + LFR)
    ├── sensevoice_dla_adapter.cpp # DLA 推理适配器
    ├── tokenizer.cpp              # 分词器实现
    ├── test_audio_only.cpp        # 仅测试音频处理（不加载DLA）
    └── main.cpp                   # 完整推理主程序
```

## 功能模块

### 1. 音频前端 (audio_frontend_impl.cpp)
- ✅ WAV 文件读取
- ✅ 立体声转单声道
- ✅ 重采样到 16kHz
- ✅ Fbank 特征提取 (80 维，使用 kaldi-native-fbank)
- ✅ LFR 处理 (80维 → 560维，合并7帧，下采样6倍)
- ✅ 已在 Android 设备上测试通过

### 2. DLA 推理适配器 (sensevoice_dla_adapter.cpp)
- ✅ Neuron Runtime API 加载
- ✅ DLA 模型加载和初始化
- ✅ 双输入支持 (特征 + prompt)
- ✅ FP16 → FP32 输出转换
- ⚠️ 待集成到完整测试

### 3. 分词器 (tokenizer.cpp)
- ✅ 词汇表加载
- ✅ CTC 解码
- ⚠️ 待测试

## 快速开始

### 1. 构建音频测试程序（不含 DLA）

```bash
# 构建
bash build_android.sh

# 部署并测试（需要连接 Android 设备）
bash deploy_and_test.sh
```

### 2. 手动测试

```bash
# 推送到设备
adb push install/android/arm64-v8a/bin/sensevoice_audio_test /data/local/tmp/
adb push install/android/arm64-v8a/bin/libc++_shared.so /data/local/tmp/
adb push <audio.wav> /data/local/tmp/

# 在设备上运行
adb shell "cd /data/local/tmp && LD_LIBRARY_PATH=. ./sensevoice_audio_test <audio.wav>"
```

## 测试结果

### 音频处理测试 ✅

测试音频：`test_en.wav` (5.86秒，16kHz)

| 步骤 | 输入 | 输出 | 状态 |
|------|------|------|------|
| WAV读取 | 93680 样本 | audio_buffer | ✅ |
| Fbank | 584 帧 | 584 × 80 | ✅ |
| LFR | 584 × 80 | 97 × 560 | ✅ |

**验证结果**：
- ✅ 特征维度正确：560 维
- ✅ 特征值范围正常：[-15.94, 6.26]
- ⚠️ 帧数：97 帧（需要 padding 到 166 帧）

## 依赖项

### 第三方库
- **kaldi-native-fbank**: `/home/xh/projects/MTK/1_third_party/kaldi_native_fbank/Android/arm64-v8a/`
  - 静态库：`libkaldi-native-fbank-core.a`
  - 头文件：`include/kaldi-native-fbank/`

### MTK SDK
- **NeuroPilot SDK**: `/home/xh/projects/MTK/0_Toolkits/neuropilot-sdk-basic-8.0.10-build20251029/neuron_sdk/`
  - 运行时库：`libneuron_runtime.so`（设备上）
  - 头文件：`include/neuron/`

### 构建工具
- **Android NDK**: r25c (`/home/xh/Android/Ndk/android-ndk-r25c`)
- **目标平台**: arm64-v8a (MT8371)

## 配置说明

### SenseVoice 模型配置
- **输入1**: 特征 `[1, 166, 560]` - 固定 166 帧（10秒音频）
- **输入2**: Prompt `[4]` - `[language_id, event1, event2, text_norm_id]`
- **输出**: Logits `[1, 170, 25055]` - 166 + 4 = 170 帧

### Prompt 配置
```cpp
// 语言 ID
auto=0, zh=3, en=4, yue=7, ja=11, ko=12, nospeech=13

// 事件类型
event1: HAPPY=1, SAD=2, ANGRY=3, NEUTRAL=4
event2: Speech=2, Music=3, Applause=4

// 文本规范化
withitn=14, woitn=15
```

## 下一步工作

- [ ] 集成 DLA 推理到完整测试程序
- [ ] 实现音频 padding（97 帧 → 166 帧）
- [ ] 端到端测试（音频 → DLA推理 → 文本输出）
- [ ] 性能优化和错误处理

## 参考资料

- 模型转换：`../SenseVoice_workspace/`
- RKNN 参考：`/home/xh/projects/sherpa-onnx/sherpa-onnx/csrc/rknn/`
- MTK 示例：`/home/xh/projects/MTK/GAI-Deployment-Toolkit-v2.0.2_clip-text-v0.1/`

## 版本历史

- **2025-01-09**: 音频前端测试通过
- **待完成**: DLA 推理集成
