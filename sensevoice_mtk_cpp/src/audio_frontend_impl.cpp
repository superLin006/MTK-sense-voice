#include "audio_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <vector>

// 使用 kaldi-native-fbank
#include "kaldi-native-fbank/csrc/online-feature.h"
#include "kaldi-native-fbank/csrc/feature-fbank.h"

int read_audio(const char *audio_path, audio_buffer_t *audio) {
    // 简单的 WAV 文件读取
    FILE *fp = fopen(audio_path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open audio file: %s\n", audio_path);
        return -1;
    }

    // 读取 WAV 头部 (简化版,假设标准格式)
    char header[44];
    if (fread(header, 1, 44, fp) != 44) {
        fprintf(stderr, "Invalid WAV file\n");
        fclose(fp);
        return -1;
    }

    // 解析头部
    audio->sample_rate = *(int32_t*)(header + 24);
    audio->num_channels = *(int16_t*)(header + 22);
    int32_t data_size = *(int32_t*)(header + 40);

    printf("WAV file info:\n");
    printf("  Sample rate: %d Hz\n", audio->sample_rate);
    printf("  Channels: %d\n", audio->num_channels);
    printf("  Data size: %d bytes\n", data_size);

    // 读取音频数据 (假设 16-bit PCM)
    int num_samples = data_size / 2;  // 16-bit = 2 bytes
    int16_t *pcm_data = (int16_t*)malloc(data_size);

    if (fread(pcm_data, 1, data_size, fp) != data_size) {
        fprintf(stderr, "Failed to read audio data\n");
        free(pcm_data);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    // 转换为 float [-1, 1]
    audio->num_samples = num_samples / audio->num_channels;
    audio->data = (float*)malloc(audio->num_samples * sizeof(float));

    if (audio->num_channels == 1) {
        // 单声道
        for (int i = 0; i < audio->num_samples; i++) {
            audio->data[i] = pcm_data[i] / 32768.0f;
        }
    } else {
        // 立体声转单声道 (取平均)
        for (int i = 0; i < audio->num_samples; i++) {
            float left = pcm_data[i * 2] / 32768.0f;
            float right = pcm_data[i * 2 + 1] / 32768.0f;
            audio->data[i] = (left + right) / 2.0f;
        }
    }

    free(pcm_data);

    printf("  Duration: %.2f seconds\n", (float)audio->num_samples / audio->sample_rate);
    printf("  Samples: %d\n", audio->num_samples);

    return 0;
}

int convert_to_mono(audio_buffer_t *audio) {
    if (audio->num_channels == 1) {
        return 0;
    }

    // 已经在 read_audio 中处理
    return 0;
}

int resample_audio(audio_buffer_t *audio, int target_rate) {
    if (audio->sample_rate == target_rate) {
        return 0;
    }

    printf("Resampling: %d Hz -> %d Hz\n", audio->sample_rate, target_rate);

    // 简单的线性插值重采样
    float ratio = (float)audio->sample_rate / target_rate;
    int new_num_samples = (int)(audio->num_samples / ratio);

    float *new_data = (float*)malloc(new_num_samples * sizeof(float));

    for (int i = 0; i < new_num_samples; i++) {
        float pos = i * ratio;
        int idx = (int)pos;
        float frac = pos - idx;

        if (idx + 1 < audio->num_samples) {
            new_data[i] = audio->data[idx] * (1.0f - frac) +
                         audio->data[idx + 1] * frac;
        } else {
            new_data[i] = audio->data[idx];
        }
    }

    free(audio->data);
    audio->data = new_data;
    audio->num_samples = new_num_samples;
    audio->sample_rate = target_rate;

    return 0;
}

int extract_fbank_features(
    const audio_buffer_t *audio,
    std::vector<float> &features,
    int *num_frames
) {
    printf("Extracting fbank features...\n");

    // 配置 kaldi fbank
    knf::FbankOptions opts;
    opts.frame_opts.samp_freq = audio->sample_rate;
    opts.frame_opts.dither = 0.0;  // 不加抖动
    opts.frame_opts.frame_shift_ms = FRAME_SHIFT_MS;
    opts.frame_opts.frame_length_ms = FRAME_LENGTH_MS;
    opts.mel_opts.num_bins = N_MELS;

    // 创建特征提取器
    knf::OnlineFbank fbank(opts);

    // 输入音频
    fbank.AcceptWaveform(audio->sample_rate, audio->data, audio->num_samples);
    fbank.InputFinished();

    *num_frames = fbank.NumFramesReady();
    printf("  Frames extracted: %d\n", *num_frames);

    // 获取特征
    features.clear();
    features.reserve(*num_frames * N_MELS);

    for (int i = 0; i < *num_frames; i++) {
        const float *frame = fbank.GetFrame(i);
        for (int j = 0; j < N_MELS; j++) {
            features.push_back(frame[j]);
        }
    }

    printf("  Feature dim: %d\n", N_MELS);
    printf("  Feature size: %zu floats\n", features.size());

    return 0;
}

int apply_lfr(
    const std::vector<float> &features,
    int num_in_frames,
    std::vector<float> &output,
    int *num_out_frames
) {
    printf("Applying LFR (m=%d, n=%d)...\n", LFR_M, LFR_N);

    if (num_in_frames < LFR_M) {
        fprintf(stderr, "Input frames (%d) less than LFR_M (%d)\n",
                num_in_frames, LFR_M);
        return -1;
    }

    // 计算 LFR 后的帧数
    // 公式: out_frames = (num_in_frames - LFR_M) / LFR_N + 1
    int out_frames = (num_in_frames - LFR_M) / LFR_N + 1;

    // 输出维度: LFR_M 帧拼接
    int out_feat_dim = N_MELS * LFR_M;  // 80 * 7 = 560

    // 分配输出缓冲区
    output.clear();
    output.resize(out_frames * out_feat_dim);

    // 填充 LFR 特征
    const float *p_in = features.data();
    float *p_out = output.data();

    for (int i = 0; i < out_frames; i++) {
        // 拼接 LFR_M=7 帧
        std::copy(p_in, p_in + out_feat_dim, p_out);

        p_out += out_feat_dim;
        p_in += LFR_N * N_MELS;  // 跳过 LFR_N=6 帧
    }

    *num_out_frames = out_frames;

    printf("  LFR output: %d frames x %d dim\n", out_frames, out_feat_dim);
    printf("  Total size: %zu floats\n", output.size());
    printf("  Frame reduction: %d -> %d (%.1f%%)\n",
           num_in_frames, out_frames,
           100.0f * out_frames / num_in_frames);

    return 0;
}
