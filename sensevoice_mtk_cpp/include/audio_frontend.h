#ifndef AUDIO_FRONTEND_H
#define AUDIO_FRONTEND_H

#include <vector>
#include <string>

// SenseVoice 音频配置
#define SAMPLE_RATE 16000
#define FRAME_LENGTH_MS 25
#define FRAME_SHIFT_MS 10
#define N_MELS 80
#define LFR_M 7    // LFR 合并帧数
#define LFR_N 6    // LFR 下采样因子

/**
 * 音频缓冲区结构
 */
typedef struct {
    float *data;          // 音频数据 (归一化到 [-1, 1])
    int num_samples;      // 样本数
    int sample_rate;      // 采样率
    int num_channels;     // 声道数
} audio_buffer_t;

/**
 * 读取音频文件
 * @param audio_path 音频文件路径
 * @param audio 输出音频缓冲区
 * @return 0 表示成功, 非0表示失败
 */
int read_audio(const char *audio_path, audio_buffer_t *audio);

/**
 * 将音频从立体声转为单声道
 * @param audio 音频缓冲区
 * @return 0 表示成功, 非0表示失败
 */
int convert_to_mono(audio_buffer_t *audio);

/**
 * 重采样音频
 * @param audio 音频缓冲区
 * @param target_rate 目标采样率
 * @return 0 表示成功, 非0表示失败
 */
int resample_audio(audio_buffer_t *audio, int target_rate);

/**
 * 提取 Fbank 特征
 * @param audio 音频缓冲区
 * @param features 输出特征向量 (num_frames x 80)
 * @param num_frames 输出帧数
 * @return 0 表示成功, 非0表示失败
 */
int extract_fbank_features(
    const audio_buffer_t *audio,
    std::vector<float> &features,
    int *num_frames
);

/**
 * 应用 LFR (Low Frame Rate) 下采样
 * @param features 输入特征 (num_in_frames x 80)
 * @param num_in_frames 输入帧数
 * @param output 输出特征 (num_out_frames x (80*LFR_M))
 * @param num_out_frames 输出帧数
 * @return 0 表示成功, 非0表示失败
 */
int apply_lfr(
    const std::vector<float> &features,
    int num_in_frames,
    std::vector<float> &output,
    int *num_out_frames
);

#endif // AUDIO_FRONTEND_H
