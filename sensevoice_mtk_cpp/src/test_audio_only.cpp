/**
 * SenseVoice 音频处理测试程序 (Android)
 * 只测试音频前端，不加载 DLA 模型
 *
 * 用法: ./sensevoice_audio_test <audio.wav>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#include "audio_frontend.h"

#define LOG_TAG "SenseVoiceAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

int main(int argc, char **argv) {
    if (argc < 2) {
        LOGE("Usage: %s <audio.wav>", argv[0]);
        printf("Usage: %s <audio.wav>\n", argv[0]);
        return -1;
    }

    const char *audio_path = argv[1];

    LOGI("========================================");
    LOGI("  SenseVoice Audio Processing Test");
    LOGI("========================================");
    printf("\n========================================\n");
    printf("  SenseVoice Audio Processing Test\n");
    printf("========================================\n\n");

    int ret;

    // 1. 读取音频
    LOGI("[1/4] Reading audio: %s", audio_path);
    printf("[1/4] Reading audio: %s\n", audio_path);

    audio_buffer_t audio;
    memset(&audio, 0, sizeof(audio_buffer_t));

    ret = read_audio(audio_path, &audio);
    if (ret != 0) {
        LOGE("❌ Failed to read audio");
        return -1;
    }
    LOGI("✅ Audio loaded: %d samples, %d Hz, %d channels, %.2f sec",
         audio.num_samples, audio.sample_rate, audio.num_channels,
         (float)audio.num_samples / audio.sample_rate);
    printf("✅ Audio loaded: %d samples, %d Hz, %d channels, %.2f sec\n\n",
           audio.num_samples, audio.sample_rate, audio.num_channels,
           (float)audio.num_samples / audio.sample_rate);

    // 2. 转换为单声道
    LOGI("[2/4] Converting to mono...");
    printf("[2/4] Converting to mono...\n");

    ret = convert_to_mono(&audio);
    if (ret != 0) {
        LOGE("❌ Failed to convert to mono");
        free(audio.data);
        return -1;
    }
    LOGI("✅ Mono conversion completed");
    printf("✅ Mono conversion completed\n\n");

    // 3. 重采样到 16kHz
    LOGI("[3/4] Resampling to %d Hz...", SAMPLE_RATE);
    printf("[3/4] Resampling to %d Hz...\n", SAMPLE_RATE);

    ret = resample_audio(&audio, SAMPLE_RATE);
    if (ret != 0) {
        LOGE("❌ Failed to resample");
        free(audio.data);
        return -1;
    }
    LOGI("✅ Resampling completed: %d samples @ %d Hz",
         audio.num_samples, audio.sample_rate);
    printf("✅ Resampling completed: %d samples @ %d Hz\n\n",
           audio.num_samples, audio.sample_rate);

    // 验证采样率
    if (audio.sample_rate != SAMPLE_RATE) {
        LOGE("❌ Sample rate mismatch: %d != %d", audio.sample_rate, SAMPLE_RATE);
        free(audio.data);
        return -1;
    }

    // 4. 提取 Fbank 特征
    LOGI("[4/4] Extracting fbank features...");
    printf("[4/4] Extracting fbank features...\n");

    std::vector<float> fbank_features;
    int num_fbank_frames = 0;

    ret = extract_fbank_features(&audio, fbank_features, &num_fbank_frames);
    if (ret != 0) {
        LOGE("❌ Failed to extract fbank features");
        free(audio.data);
        return -1;
    }
    LOGI("✅ Fbank extracted: %d frames x %d dims = %zu floats",
         num_fbank_frames, N_MELS, fbank_features.size());
    printf("✅ Fbank extracted: %d frames x %d dims = %zu floats\n\n",
           num_fbank_frames, N_MELS, fbank_features.size());

    // 5. 应用 LFR
    LOGI("[5/5] Applying LFR (m=%d, n=%d)...", LFR_M, LFR_N);
    printf("[5/5] Applying LFR (m=%d, n=%d)...\n", LFR_M, LFR_N);

    std::vector<float> lfr_features;
    int num_lfr_frames = 0;

    ret = apply_lfr(fbank_features, num_fbank_frames, lfr_features, &num_lfr_frames);
    if (ret != 0) {
        LOGE("❌ Failed to apply LFR");
        free(audio.data);
        return -1;
    }
    LOGI("✅ LFR applied: %d frames x %d dims = %zu floats",
         num_lfr_frames, N_MELS * LFR_M, lfr_features.size());
    printf("✅ LFR applied: %d frames x %d dims = %zu floats\n\n",
           num_lfr_frames, N_MELS * LFR_M, lfr_features.size());

    // 6. 验证输出
    LOGI("========================================");
    LOGI("  Verification Results");
    LOGI("========================================");
    printf("========================================\n");
    printf("  Verification Results\n");
    printf("========================================\n");

    int expected_feat_dim = N_MELS * LFR_M;  // 80 * 7 = 560
    int actual_feat_dim = lfr_features.size() / num_lfr_frames;

    printf("Fbank frames:    %d\n", num_fbank_frames);
    printf("Fbank dim:       %d\n", N_MELS);
    printf("LFR frames:      %d\n", num_lfr_frames);
    printf("LFR dim:         %d (expected: %d)\n", actual_feat_dim, expected_feat_dim);
    printf("Total features:  %zu floats\n", lfr_features.size());

    LOGI("Fbank frames: %d, Fbank dim: %d", num_fbank_frames, N_MELS);
    LOGI("LFR frames: %d, LFR dim: %d (expected: %d)",
         num_lfr_frames, actual_feat_dim, expected_feat_dim);

    bool success = true;

    // 验证维度
    if (actual_feat_dim != expected_feat_dim) {
        LOGE("❌ LFR feature dimension mismatch!");
        LOGE("   Expected: %d, Got: %d", expected_feat_dim, actual_feat_dim);
        fprintf(stderr, "\n❌ ERROR: LFR feature dimension mismatch!\n");
        fprintf(stderr, "   Expected: %d\n", expected_feat_dim);
        fprintf(stderr, "   Got:      %d\n", actual_feat_dim);
        success = false;
    } else {
        LOGI("✅ Feature dimension correct: %d", actual_feat_dim);
    }

    // 检查帧数
    if (num_lfr_frames > 166) {
        LOGW("⚠️  Too many LFR frames (%d > 166)", num_lfr_frames);
        LOGW("   Model expects fixed 166 frames (10 seconds)");
        fprintf(stderr, "\n⚠️  WARNING: Too many LFR frames (%d > 166)\n", num_lfr_frames);
        fprintf(stderr, "   Model expects fixed 166 frames (10 seconds)\n");
        fprintf(stderr, "   You may need to truncate the audio\n");
    } else if (num_lfr_frames < 166) {
        LOGW("⚠️  Fewer LFR frames (%d < 166)", num_lfr_frames);
        LOGW("   Will be padded to 166 frames");
        fprintf(stderr, "\n⚠️  Fewer LFR frames (%d < 166)\n", num_lfr_frames);
        fprintf(stderr, "   Will be padded to 166 frames\n");
    } else {
        LOGI("✅ Frame count perfect: %d", num_lfr_frames);
    }

    // 显示前几个和后几个特征值
    printf("\nFirst 10 feature values:\n");
    LOGI("First 10 feature values:");
    for (int i = 0; i < 10 && i < lfr_features.size(); i++) {
        printf("  [%d] %.6f\n", i, lfr_features[i]);
        if (i < 5) {
            LOGI("  [%d] %.6f", i, lfr_features[i]);
        }
    }

    printf("\nLast 10 feature values:\n");
    LOGI("Last 10 feature values:");
    for (int i = 0; i < 10 && i < lfr_features.size(); i++) {
        int idx = lfr_features.size() - 10 + i;
        printf("  [%d] %.6f\n", idx, lfr_features[idx]);
        if (i < 5) {
            LOGI("  [%d] %.6f", idx, lfr_features[idx]);
        }
    }

    // 统计信息
    float sum = 0.0f;
    float min_val = lfr_features[0];
    float max_val = lfr_features[0];
    for (size_t i = 0; i < lfr_features.size(); i++) {
        sum += lfr_features[i];
        if (lfr_features[i] < min_val) min_val = lfr_features[i];
        if (lfr_features[i] > max_val) max_val = lfr_features[i];
    }
    float mean = sum / lfr_features.size();

    printf("\nFeature statistics:\n");
    printf("  Mean:   %.6f\n", mean);
    printf("  Min:    %.6f\n", min_val);
    printf("  Max:    %.6f\n", max_val);
    printf("  Range:  %.6f\n", max_val - min_val);

    LOGI("Feature stats - Mean: %.6f, Min: %.6f, Max: %.6f",
         mean, min_val, max_val);

    printf("\n");
    if (success) {
        LOGI("✅ All audio processing tests passed!");
        printf("✅ All audio processing tests passed!\n\n");
    } else {
        LOGE("❌ Some tests failed!");
        printf("❌ Some tests failed!\n\n");
    }

    // 清理
    free(audio.data);

    return success ? 0 : -1;
}
