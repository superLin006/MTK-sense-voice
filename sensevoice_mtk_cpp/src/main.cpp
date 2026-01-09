#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <chrono>

#include "sensevoice_dla_adapter.h"
#include "audio_frontend.h"
#include "tokenizer.h"

void print_usage(const char *program_name) {
    printf("Usage: %s <model.dla> <tokens.txt> <audio.wav> [language] [text_norm]\n", program_name);
    printf("\n");
    printf("Arguments:\n");
    printf("  model.dla   - SenseVoice DLA model file\n");
    printf("  tokens.txt  - Tokens vocabulary file\n");
    printf("  audio.wav   - Input audio file (16kHz WAV recommended)\n");
    printf("  language    - Language ID (optional, default: 0=auto)\n");
    printf("                0=auto, 3=zh, 4=en, 5=yue, 6=ja, 7=ko\n");
    printf("  text_norm   - Text normalization (optional, default: 15)\n");
    printf("                15=with punctuation, 14=without punctuation\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s models/sensevoice.dla models/tokens.txt test.wav 3 15\n", program_name);
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) {
        print_usage(argv[0]);
        return -1;
    }

    const char *model_path = argv[1];
    const char *tokens_path = argv[2];
    const char *audio_path = argv[3];
    int language = (argc >= 5) ? atoi(argv[4]) : 0;      // 默认 auto
    int text_norm = (argc >= 6) ? atoi(argv[5]) : 15;    // 默认有标点

    printf("\n");
    printf("===========================================\n");
    printf("    SenseVoice MTK NPU Inference Demo    \n");
    printf("===========================================\n");
    printf("Model:     %s\n", model_path);
    printf("Tokens:    %s\n", tokens_path);
    printf("Audio:     %s\n", audio_path);
    printf("Language:  %d\n", language);
    printf("Text Norm: %d\n", text_norm);
    printf("===========================================\n\n");

    int ret;
    sensevoice_dla_context_t dla_ctx;
    memset(&dla_ctx, 0, sizeof(sensevoice_dla_context_t));

    // 1. 初始化 DLA 模型
    printf("[1/5] Initializing DLA model...\n");
    auto start_time = std::chrono::high_resolution_clock::now();

    ret = init_sensevoice_model(model_path, &dla_ctx);
    if (ret != 0) {
        fprintf(stderr, "Failed to initialize SenseVoice model\n");
        return -1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    printf("✅ Model initialized in %lld ms\n\n", (long long)duration.count());

    // 2. 加载分词器
    printf("[2/5] Loading tokenizer...\n");
    SenseVoiceTokenizer tokenizer;
    if (!tokenizer.LoadVocab(tokens_path)) {
        fprintf(stderr, "Failed to load tokenizer\n");
        release_sensevoice_model(&dla_ctx);
        return -1;
    }
    printf("Vocabulary size: %d\n\n", tokenizer.GetVocabSize());

    // 3. 读取音频并提取特征
    printf("[3/5] Processing audio...\n");

    audio_buffer_t audio;
    memset(&audio, 0, sizeof(audio_buffer_t));

    // 读取音频文件
    if (read_audio(audio_path, &audio) != 0) {
        fprintf(stderr, "Failed to read audio file\n");
        tokenizer.~SenseVoiceTokenizer();
        release_sensevoice_model(&dla_ctx);
        return -1;
    }

    // 转换为单声道
    convert_to_mono(&audio);

    // 重采样到 16kHz
    resample_audio(&audio, SAMPLE_RATE);

    // 提取 Fbank 特征
    std::vector<float> fbank_features;
    int num_fbank_frames = 0;
    if (extract_fbank_features(&audio, fbank_features, &num_fbank_frames) != 0) {
        fprintf(stderr, "Failed to extract features\n");
        free(audio.data);
        tokenizer.~SenseVoiceTokenizer();
        release_sensevoice_model(&dla_ctx);
        return -1;
    }

    // 应用 LFR 下采样
    std::vector<float> lfr_features;
    int num_lfr_frames = 0;
    if (apply_lfr(fbank_features, num_fbank_frames, lfr_features, &num_lfr_frames) != 0) {
        fprintf(stderr, "Failed to apply LFR\n");
        free(audio.data);
        tokenizer.~SenseVoiceTokenizer();
        release_sensevoice_model(&dla_ctx);
        return -1;
    }

    printf("Audio processing complete: %d frames x 560 dim\n", num_lfr_frames);
    printf("Total feature size: %zu floats (%.2f MB)\n\n",
           lfr_features.size(), lfr_features.size() * sizeof(float) / (1024.0f * 1024.0f));

    // 释放音频缓冲区
    free(audio.data);

    int test_num_frames = num_lfr_frames;
    std::vector<float> &test_features = lfr_features;

    printf("DEBUG: test_num_frames=%d, test_features.size()=%zu\n",
           test_num_frames, test_features.size());
    printf("DEBUG: Expected size: %d * 560 = %d floats\n",
           test_num_frames, test_num_frames * 560);

    // 4. 执行推理
    printf("[4/5] Running inference on MTK NPU...\n");
    start_time = std::chrono::high_resolution_clock::now();

    float *output = nullptr;
    int output_frames = 0;

    ret = inference_sensevoice_model(
        &dla_ctx,
        test_features.data(),
        test_num_frames,
        language,
        text_norm,
        &output,
        &output_frames
    );

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    if (ret != 0) {
        fprintf(stderr, "Inference failed\n");
        release_sensevoice_model(&dla_ctx);
        return -1;
    }

    printf("✅ Inference completed in %lld ms\n\n", (long long)duration.count());

    // 5. 解码输出
    printf("[5/5] Decoding output...\n");
    std::string result = tokenizer.DecodeLogits(output, output_frames, tokenizer.GetVocabSize());

    printf("\n");
    printf("===========================================\n");
    printf("Recognition Result:\n");
    printf("-------------------------------------------\n");
    printf("%s\n", result.empty() ? "(empty)" : result.c_str());
    printf("===========================================\n\n");

    // 统计信息
    float rtf = duration.count() / 1000.0f / (test_num_frames * 0.04f);  // 假设每帧 40ms
    printf("Performance:\n");
    printf("  Inference time: %lld ms\n", (long long)duration.count());
    printf("  Real-time factor: %.2fx\n", rtf);
    printf("\n");

    // 清理
    if (output) {
        free(output);
    }

    release_sensevoice_model(&dla_ctx);

    printf("✅ All done!\n\n");
    return 0;
}
