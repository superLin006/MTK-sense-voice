#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chrono>

#include "sensevoice_dla_adapter.h"
#include "tokenizer.h"

/**
 * 推理引擎 - 独立程序
 * 输入: 二进制特征文件 (.bin)
 * 输出: 识别文本
 */
int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) {
        printf("Usage: %s <model.dla> <tokens.txt> <features.bin> [language] [text_norm]\n", argv[0]);
        printf("\n");
        printf("Arguments:\n");
        printf("  model.dla    - SenseVoice DLA model file\n");
        printf("  tokens.txt   - Tokens vocabulary file\n");
        printf("  features.bin - Input features (from extract_features)\n");
        printf("  language     - Language ID (optional, default: 0=auto)\n");
        printf("                 0=auto, 3=zh, 4=en, 5=yue, 6=ja, 7=ko\n");
        printf("  text_norm    - Text normalization (optional, default: 15)\n");
        printf("                 15=with punctuation, 14=without punctuation\n");
        printf("\n");
        return -1;
    }

    const char *model_path = argv[1];
    const char *tokens_path = argv[2];
    const char *features_path = argv[3];
    int language = (argc >= 5) ? atoi(argv[4]) : 0;
    int text_norm = (argc >= 6) ? atoi(argv[5]) : 15;

    printf("\n");
    printf("===========================================\n");
    printf("  SenseVoice MTK NPU Inference Engine\n");
    printf("===========================================\n");
    printf("Model:    %s\n", model_path);
    printf("Tokens:   %s\n", tokens_path);
    printf("Features: %s\n", features_path);
    printf("Language: %d\n", language);
    printf("TextNorm: %d\n", text_norm);
    printf("===========================================\n\n");

    int ret;

    // 1. 加载特征
    printf("[1/4] Loading features from file...\n");
    FILE *fp = fopen(features_path, "rb");
    if (!fp) {
        fprintf(stderr, "❌ Failed to open features file\n");
        return -1;
    }

    // 读取元数据
    int num_frames, feat_dim;
    if (fread(&num_frames, sizeof(int), 1, fp) != 1 ||
        fread(&feat_dim, sizeof(int), 1, fp) != 1) {
        fprintf(stderr, "❌ Failed to read feature metadata\n");
        fclose(fp);
        return -1;
    }

    if (feat_dim != 560) {
        fprintf(stderr, "❌ Invalid feature dimension: %d (expected 560)\n", feat_dim);
        fclose(fp);
        return -1;
    }

    // 读取特征数据
    std::vector<float> features(num_frames * feat_dim);
    size_t read_count = fread(features.data(), sizeof(float), features.size(), fp);
    fclose(fp);

    if (read_count != features.size()) {
        fprintf(stderr, "❌ Failed to read all features: got %zu, expected %zu\n",
                read_count, features.size());
        return -1;
    }

    printf("✅ Features loaded: %d frames x %d dim\n\n", num_frames, feat_dim);

    printf("DEBUG: About to initialize DLA model...\n");
    printf("DEBUG: model_path = %s\n", model_path);

    // 2. 初始化 DLA 模型
    printf("[2/4] Initializing DLA model...\n");
    auto start_time = std::chrono::high_resolution_clock::now();

    sensevoice_dla_context_t dla_ctx;
    memset(&dla_ctx, 0, sizeof(sensevoice_dla_context_t));

    ret = init_sensevoice_model(model_path, &dla_ctx);
    if (ret != 0) {
        fprintf(stderr, "❌ Failed to initialize model\n");
        return -1;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    printf("✅ Model initialized in %lld ms\n\n", (long long)duration.count());

    // 3. 加载分词器
    printf("[3/4] Loading tokenizer...\n");
    SenseVoiceTokenizer tokenizer;
    if (!tokenizer.LoadVocab(tokens_path)) {
        fprintf(stderr, "❌ Failed to load tokenizer\n");
        release_sensevoice_model(&dla_ctx);
        return -1;
    }
    printf("✅ Tokenizer loaded (vocab size: %d)\n\n", tokenizer.GetVocabSize());

    // 4. 执行推理
    printf("[4/4] Running inference on MTK NPU...\n");
    start_time = std::chrono::high_resolution_clock::now();

    float *output = nullptr;
    int output_frames = 0;

    ret = inference_sensevoice_model(
        &dla_ctx,
        features.data(),
        num_frames,
        language,
        text_norm,
        &output,
        &output_frames
    );

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    if (ret != 0) {
        fprintf(stderr, "❌ Inference failed\n");
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

    // 性能统计
    float rtf = duration.count() / 1000.0f / (num_frames * 0.04f);
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
