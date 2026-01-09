#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "audio_frontend.h"

/**
 * 特征提取器 - 独立程序
 * 输入: WAV 音频文件
 * 输出: 二进制特征文件 (.bin)
 */
int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input.wav> <output.bin>\n", argv[0]);
        printf("\n");
        printf("Extract acoustic features from audio file for SenseVoice inference.\n");
        printf("Output format: binary file with [num_frames, 560] float32 array\n");
        printf("\n");
        return -1;
    }

    const char *audio_path = argv[1];
    const char *output_path = argv[2];

    printf("\n");
    printf("===========================================\n");
    printf("  SenseVoice Feature Extractor\n");
    printf("===========================================\n");
    printf("Input:  %s\n", audio_path);
    printf("Output: %s\n", output_path);
    printf("===========================================\n\n");

    // 1. 读取音频
    printf("[1/4] Reading audio file...\n");
    audio_buffer_t audio;
    memset(&audio, 0, sizeof(audio_buffer_t));

    if (read_audio(audio_path, &audio) != 0) {
        fprintf(stderr, "❌ Failed to read audio file\n");
        return -1;
    }
    printf("✅ Audio loaded: %d samples, %.2f seconds\n\n",
           audio.num_samples, (float)audio.num_samples / audio.sample_rate);

    // 2. 预处理
    printf("[2/4] Preprocessing audio...\n");
    convert_to_mono(&audio);
    resample_audio(&audio, SAMPLE_RATE);
    printf("✅ Preprocessed: mono, %d Hz\n\n", SAMPLE_RATE);

    // 3. 提取 Fbank 特征
    printf("[3/4] Extracting Fbank features...\n");
    std::vector<float> fbank_features;
    int num_fbank_frames = 0;

    if (extract_fbank_features(&audio, fbank_features, &num_fbank_frames) != 0) {
        fprintf(stderr, "❌ Failed to extract features\n");
        free(audio.data);
        return -1;
    }
    printf("✅ Fbank extracted: %d frames x 80 dim\n\n", num_fbank_frames);

    // 4. 应用 LFR 下采样
    printf("[4/4] Applying LFR downsampling...\n");
    std::vector<float> lfr_features;
    int num_lfr_frames = 0;

    if (apply_lfr(fbank_features, num_fbank_frames, lfr_features, &num_lfr_frames) != 0) {
        fprintf(stderr, "❌ Failed to apply LFR\n");
        free(audio.data);
        return -1;
    }
    printf("✅ LFR applied: %d frames x 560 dim\n\n", num_lfr_frames);

    free(audio.data);

    // 5. 保存特征到文件
    printf("Saving features to %s...\n", output_path);
    FILE *fp = fopen(output_path, "wb");
    if (!fp) {
        fprintf(stderr, "❌ Failed to open output file\n");
        return -1;
    }

    // 写入元数据
    fwrite(&num_lfr_frames, sizeof(int), 1, fp);
    int feat_dim = 560;
    fwrite(&feat_dim, sizeof(int), 1, fp);

    // 写入特征数据
    fwrite(lfr_features.data(), sizeof(float), lfr_features.size(), fp);
    fclose(fp);

    printf("✅ Features saved successfully\n");
    printf("\n");
    printf("===========================================\n");
    printf("Output Summary:\n");
    printf("  Frames:     %d\n", num_lfr_frames);
    printf("  Dimensions: 560\n");
    printf("  File size:  %.2f KB\n", (8 + lfr_features.size() * 4) / 1024.0f);
    printf("===========================================\n\n");

    return 0;
}
