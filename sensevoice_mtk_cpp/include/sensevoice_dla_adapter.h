#ifndef SENSEVOICE_DLA_ADAPTER_H
#define SENSEVOICE_DLA_ADAPTER_H

#include <stdint.h>
#include <vector>
#include <string>

// SenseVoice MTK DLA 上下文
typedef struct {
    void* runtime;              // NeuronRuntime* (通过 Runtime API 创建)
    void* model_data;           // dla 文件内容
    size_t model_size;
    int n_input;
    int n_output;
    size_t input_size;          // 输入张量大小(字节)
    size_t output_size;         // 输出张量大小(字节)
} sensevoice_dla_context_t;

/**
 * 初始化 SenseVoice DLA 模型
 * @param model_path DLA 模型文件路径
 * @param app_ctx 上下文指针
 * @return 0 表示成功, 非0表示失败
 */
int init_sensevoice_model(const char *model_path, sensevoice_dla_context_t *app_ctx);

/**
 * 释放 SenseVoice DLA 模型
 * @param app_ctx 上下文指针
 * @return 0 表示成功, 非0表示失败
 */
int release_sensevoice_model(sensevoice_dla_context_t *app_ctx);

/**
 * 执行 SenseVoice 推理
 * @param app_ctx DLA 上下文
 * @param features 输入特征 (num_frames x 80)
 * @param num_frames 特征帧数
 * @param language 语言 ID (如 0=auto, 3=zh, 4=en 等)
 * @param text_norm 文本归一化选项 (0=无, 15=有标点, 等)
 * @param output 输出 logits (num_output_frames x vocab_size)
 * @param output_frames 输出帧数 (指针)
 * @return 0 表示成功, 非0表示失败
 */
int inference_sensevoice_model(
    sensevoice_dla_context_t *app_ctx,
    const float *features,
    int num_frames,
    int language,
    int text_norm,
    float **output,
    int *output_frames
);

#endif // SENSEVOICE_DLA_ADAPTER_H
