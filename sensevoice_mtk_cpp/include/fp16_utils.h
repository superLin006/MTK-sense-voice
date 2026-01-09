#ifndef FP16_UTILS_H
#define FP16_UTILS_H

#include <stdint.h>
#include <cmath>

/**
 * 将 FP16 (半精度浮点数) 转换为 FP32
 * @param fp16 输入的 FP16 值 (uint16_t)
 * @return FP32 浮点数
 */
inline float fp16_to_fp32(uint16_t fp16) {
    uint32_t sign = (fp16 & 0x8000) << 16;
    uint32_t exponent = (fp16 & 0x7C00) >> 10;
    uint32_t mantissa = (fp16 & 0x03FF);

    uint32_t fp32;

    if (exponent == 0) {
        if (mantissa == 0) {
            // Zero
            fp32 = sign;
        } else {
            // Denormalized number
            exponent = 1;
            while (!(mantissa & 0x0400)) {
                mantissa <<= 1;
                exponent--;
            }
            mantissa &= 0x03FF;
            fp32 = sign | ((exponent + (127 - 15)) << 23) | (mantissa << 13);
        }
    } else if (exponent == 0x1F) {
        // Infinity or NaN
        fp32 = sign | 0x7F800000 | (mantissa << 13);
    } else {
        // Normalized number
        fp32 = sign | ((exponent + (127 - 15)) << 23) | (mantissa << 13);
    }

    float result;
    memcpy(&result, &fp32, sizeof(float));
    return result;
}

/**
 * 将 FP16 数组转换为 FP32 数组
 * @param fp16_array 输入 FP16 数组
 * @param fp32_array 输出 FP32 数组
 * @param count 元素数量
 */
inline void fp16_to_fp32_array(const uint16_t *fp16_array, float *fp32_array, int count) {
    for (int i = 0; i < count; i++) {
        fp32_array[i] = fp16_to_fp32(fp16_array[i]);
    }
}

#endif // FP16_UTILS_H
