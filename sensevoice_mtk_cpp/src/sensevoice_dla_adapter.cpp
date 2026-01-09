#include "sensevoice_dla_adapter.h"
#include "fp16_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

// Neuron Runtime API 类型定义
typedef enum {
    Auto   = 0,
    Single = 1,
    Dual   = 2,
} MDLACoreMode;

typedef struct {
    uint32_t deviceKind;
    MDLACoreMode MDLACoreOption;
    uint8_t CPUThreadNum;
    bool suppressInputConversion;
    bool suppressOutputConversion;
} EnvOptions;

const unsigned char kEnvOptHardware = 1 << 2;

typedef enum {
    NEURONRUNTIME_PREFER_PERFORMANCE = 0,
    NEURONRUNTIME_PREFER_POWER,
    NEURONRUNTIME_HINT_TURBO_BOOST,
} RuntimeAPIQoSPreference;

typedef enum {
    NEURONRUNTIME_PRIORITY_HIGH = 2,
} RuntimeAPIQoSPriority;

typedef struct {
    int ionFd;  // -1 表示非 ION 缓冲区
} BufferAttribute;

typedef struct {
    RuntimeAPIQoSPreference preference;
    RuntimeAPIQoSPriority priority;
    uint8_t boostValue;
    uint8_t maxBoostValue;
    uint8_t minBoostValue;
    uint16_t deadline;
    uint16_t abortTime;
    int32_t delayedPowerOffTime;
    void* profiledQoSData;
} QoSOptions;

// Neuron Runtime API 函数指针
typedef int (*FnNeuronRuntime_create_with_options)(const char*, const EnvOptions*, void**);
typedef int (*FnNeuronRuntime_loadNetworkFromBuffer)(void*, const void*, size_t);
typedef int (*FnNeuronRuntime_setInput)(void*, uint64_t, const void*, size_t, BufferAttribute);
typedef int (*FnNeuronRuntime_setOutput)(void*, uint64_t, void*, size_t, BufferAttribute);
typedef int (*FnNeuronRuntime_inference)(void*);
typedef int (*FnNeuronRuntime_setQoSOption)(void*, const QoSOptions*);
typedef void (*FnNeuronRuntime_release)(void*);
typedef int (*FnNeuronRuntime_getInputPaddedSize)(void*, uint64_t, size_t*);
typedef int (*FnNeuronRuntime_getOutputPaddedSize)(void*, uint64_t, size_t*);

// 全局函数指针
static FnNeuronRuntime_create_with_options fnNeuronRuntime_create_with_options = nullptr;
static FnNeuronRuntime_loadNetworkFromBuffer fnNeuronRuntime_loadNetworkFromBuffer = nullptr;
static FnNeuronRuntime_setInput fnNeuronRuntime_setInput = nullptr;
static FnNeuronRuntime_setOutput fnNeuronRuntime_setOutput = nullptr;
static FnNeuronRuntime_inference fnNeuronRuntime_inference = nullptr;
static FnNeuronRuntime_setQoSOption fnNeuronRuntime_setQoSOption = nullptr;
static FnNeuronRuntime_release fnNeuronRuntime_release = nullptr;
static FnNeuronRuntime_getInputPaddedSize fnNeuronRuntime_getInputPaddedSize = nullptr;
static FnNeuronRuntime_getOutputPaddedSize fnNeuronRuntime_getOutputPaddedSize = nullptr;

#define NEURON_NO_ERROR 0

// 加载 Neuron Runtime API
static bool load_neuron_runtime_api() {
    static bool loaded = false;
    if (loaded) return true;

    void* handle = dlopen("libneuron_runtime.so", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Failed to load libneuron_runtime.so: %s\n", dlerror());
        return false;
    }

    #define LOAD_FUNC(name) \
        fn##name = (Fn##name)dlsym(handle, #name); \
        if (!fn##name) { \
            fprintf(stderr, "Failed to load function: %s\n", #name); \
            dlclose(handle); \
            return false; \
        }

    LOAD_FUNC(NeuronRuntime_create_with_options);
    LOAD_FUNC(NeuronRuntime_loadNetworkFromBuffer);
    LOAD_FUNC(NeuronRuntime_setInput);
    LOAD_FUNC(NeuronRuntime_setOutput);
    LOAD_FUNC(NeuronRuntime_inference);
    LOAD_FUNC(NeuronRuntime_setQoSOption);
    LOAD_FUNC(NeuronRuntime_release);
    LOAD_FUNC(NeuronRuntime_getInputPaddedSize);
    LOAD_FUNC(NeuronRuntime_getOutputPaddedSize);

    #undef LOAD_FUNC

    loaded = true;
    printf("✅ Neuron Runtime API loaded successfully\n");
    return true;
}

// 读取 .dla 文件到内存
static void* load_model_file(const char* path, size_t* size) {
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open model file: %s\n", path);
        return nullptr;
    }

    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    void* data = malloc(*size);
    if (!data || fread(data, 1, *size, fp) != *size) {
        fprintf(stderr, "Failed to read model file\n");
        free(data);
        fclose(fp);
        return nullptr;
    }

    fclose(fp);
    return data;
}

// 初始化 SenseVoice DLA 模型
int init_sensevoice_model(const char *model_path, sensevoice_dla_context_t *app_ctx) {
    int ret;

    printf("Loading SenseVoice DLA model: %s\n", model_path);

    // 0. 加载 Neuron Runtime API
    if (!load_neuron_runtime_api()) {
        fprintf(stderr, "Failed to load Neuron Runtime API\n");
        return -1;
    }

    // 1. 加载 .dla 文件到内存
    app_ctx->model_data = load_model_file(model_path, &app_ctx->model_size);
    if (!app_ctx->model_data) {
        return -1;
    }
    printf("Loaded DLA file: %zu bytes (%.2f MB)\n",
           app_ctx->model_size, app_ctx->model_size / (1024.0 * 1024.0));

    // 2. 配置环境选项
    EnvOptions envOptions = {};
    envOptions.deviceKind = kEnvOptHardware;
    envOptions.MDLACoreOption = MDLACoreMode::Auto;
    envOptions.CPUThreadNum = 4;
    envOptions.suppressInputConversion = false;
    envOptions.suppressOutputConversion = false;

    // 3. 创建 Neuron Runtime
    void* runtime = nullptr;
    const char* options = "--apusys-config \"{ \\\"high_addr\\\": true, \\\"import_forever\\\": true }\"";

    printf("Creating NeuronRuntime...\n");
    ret = fnNeuronRuntime_create_with_options(options, &envOptions, &runtime);
    if (ret != NEURON_NO_ERROR) {
        fprintf(stderr, "❌ NeuronRuntime_create_with_options failed: %d\n", ret);
        free(app_ctx->model_data);
        return -1;
    }
    printf("✅ NeuronRuntime created successfully\n");

    // 4. 从缓冲区加载网络模型
    printf("Loading network from buffer...\n");
    ret = fnNeuronRuntime_loadNetworkFromBuffer(runtime, app_ctx->model_data, app_ctx->model_size);
    if (ret != NEURON_NO_ERROR) {
        fprintf(stderr, "❌ NeuronRuntime_loadNetworkFromBuffer failed: %d\n", ret);
        fnNeuronRuntime_release(runtime);
        free(app_ctx->model_data);
        return -1;
    }
    printf("✅ Network loaded successfully\n");

    // 5. 设置 QoS 选项
    QoSOptions qosOptions = {};
    qosOptions.preference = NEURONRUNTIME_PREFER_PERFORMANCE;
    qosOptions.priority = NEURONRUNTIME_PRIORITY_HIGH;
    qosOptions.boostValue = 100;
    qosOptions.profiledQoSData = nullptr;

    ret = fnNeuronRuntime_setQoSOption(runtime, &qosOptions);
    if (ret != NEURON_NO_ERROR) {
        fprintf(stderr, "Warning: NeuronRuntime_setQoSOption failed: %d\n", ret);
    }

    // 6. 查询输入输出大小
    size_t input_size = 0;
    size_t output_size = 0;

    ret = fnNeuronRuntime_getInputPaddedSize(runtime, 0, &input_size);
    if (ret == NEURON_NO_ERROR) {
        app_ctx->input_size = input_size;
        printf("Input tensor size: %zu bytes\n", input_size);
    } else {
        fprintf(stderr, "Warning: Failed to get input size\n");
    }

    ret = fnNeuronRuntime_getOutputPaddedSize(runtime, 0, &output_size);
    if (ret == NEURON_NO_ERROR) {
        app_ctx->output_size = output_size;
        printf("Output tensor size: %zu bytes\n", output_size);
    } else {
        fprintf(stderr, "Warning: Failed to get output size\n");
    }

    app_ctx->runtime = runtime;
    app_ctx->n_input = 1;   // SenseVoice 只有一个输入(特征)
    app_ctx->n_output = 1;  // 一个输出(logits)

    printf("✅ SenseVoice model initialized successfully\n");
    return 0;
}

// 释放 SenseVoice DLA 模型
int release_sensevoice_model(sensevoice_dla_context_t *app_ctx) {
    if (app_ctx->runtime) {
        fnNeuronRuntime_release(app_ctx->runtime);
        app_ctx->runtime = nullptr;
    }

    if (app_ctx->model_data) {
        free(app_ctx->model_data);
        app_ctx->model_data = nullptr;
    }

    printf("SenseVoice model released\n");
    return 0;
}

// 执行 SenseVoice 推理
int inference_sensevoice_model(
    sensevoice_dla_context_t *app_ctx,
    const float *features,
    int num_frames,
    int language,
    int text_norm,
    float **output,
    int *output_frames
) {
    int ret;

    if (!app_ctx || !app_ctx->runtime) {
        fprintf(stderr, "Invalid context or runtime not initialized\n");
        return -1;
    }

    printf("\n=== SenseVoice Inference ===\n");
    printf("Input frames: %d\n", num_frames);
    printf("Language ID: %d\n", language);
    printf("Text norm: %d\n", text_norm);

    // 准备输入张量
    // SenseVoice 输入: (1, num_frames, 560) 其中 560 = 80 * 7 (LFR)
    // 注意: 这里假设输入已经过 LFR 处理
    size_t input_bytes = num_frames * 560 * sizeof(float);

    BufferAttribute attr = {-1};  // 非 ION 缓冲区

    // 设置输入
    ret = fnNeuronRuntime_setInput(app_ctx->runtime, 0, features, input_bytes, attr);
    if (ret != NEURON_NO_ERROR) {
        fprintf(stderr, "❌ setInput failed: %d\n", ret);
        return -1;
    }

    // 分配输出缓冲区
    // SenseVoice 输出: (1, num_output_frames, vocab_size)
    // 注意: MTK DLA 输出为 FP16 格式
    int vocab_size = 25055;  // SenseVoice 词汇表大小
    size_t output_bytes = app_ctx->output_size;

    // DLA 输出 FP16 缓冲区
    uint16_t *output_buffer_fp16 = (uint16_t*)malloc(output_bytes);
    if (!output_buffer_fp16) {
        fprintf(stderr, "Failed to allocate output buffer\n");
        return -1;
    }

    ret = fnNeuronRuntime_setOutput(app_ctx->runtime, 0, output_buffer_fp16, output_bytes, attr);
    if (ret != NEURON_NO_ERROR) {
        fprintf(stderr, "❌ setOutput failed: %d\n", ret);
        free(output_buffer_fp16);
        return -1;
    }

    // 执行推理
    printf("Running inference...\n");
    ret = fnNeuronRuntime_inference(app_ctx->runtime);
    if (ret != NEURON_NO_ERROR) {
        fprintf(stderr, "❌ inference failed: %d\n", ret);
        free(output_buffer_fp16);
        return -1;
    }
    printf("✅ Inference completed successfully\n");

    // 转换 FP16 到 FP32
    int total_elements = output_bytes / sizeof(uint16_t);
    *output_frames = total_elements / vocab_size;

    float *output_buffer_fp32 = (float*)malloc(total_elements * sizeof(float));
    if (!output_buffer_fp32) {
        fprintf(stderr, "Failed to allocate FP32 output buffer\n");
        free(output_buffer_fp16);
        return -1;
    }

    printf("Converting FP16 to FP32 (%d elements)...\n", total_elements);
    fp16_to_fp32_array(output_buffer_fp16, output_buffer_fp32, total_elements);

    free(output_buffer_fp16);

    // 返回 FP32 输出
    *output = output_buffer_fp32;

    printf("Output frames: %d\n", *output_frames);
    printf("========================\n\n");

    return 0;
}
