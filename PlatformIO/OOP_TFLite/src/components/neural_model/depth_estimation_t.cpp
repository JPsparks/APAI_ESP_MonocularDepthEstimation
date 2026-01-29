#include "depth_estimation_t.h"
#include <math.h>


DepthEstimationT::DepthEstimationT() {
    // Heuristic allocation for Tensor Arena
    this->kTensorArenaSize = MODEL_NAME_LEN * 4;
}

TfLiteTensor* DepthEstimationT::inference(uint8_t* input_data) {
    if (!this->interpreter) {
        log("Interpreter is missing!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    }
    TfLiteTensor* input_tensor = this->interpreter->input(0);

    float scale = input_tensor->params.scale;
    int32_t zero_point = input_tensor->params.zero_point;

    // Safety checks: use defaults if metadata is missing
    if (scale == 0.0f) { scale = 1.0f; zero_point = 0; }
    
    int8_t* dst = input_tensor->data.int8;
    const uint8_t* src = input_data;
    size_t count = input_tensor->bytes;

    // Pre-calculate multiplier to avoid slow division in loop
    // Remind: (pixel / 255.0) / scale + zero_point
    float effective_scale_inv = 1.0f / (255.0f * scale);

    for (size_t i = 0; i < count; i++) {
        // Normalize and quantize
        float val = (float)src[i] * effective_scale_inv + zero_point;
        
        // Cast to int32 for bounds checking
        // ! float need 32 bit, this is why we pass by int32
        // essentially, this is usefull to cut off the decimal part
        int32_t val_q = (int32_t)val; 
        
        // Clamp to int8 range [-128, 127]
        if (val_q < -128) val_q = -128;
        else if (val_q > 127) val_q = 127;
        
        dst[i] = (int8_t)val_q;
    }

    if (interpreter->Invoke() != kTfLiteOk) {
        log("Error along inference!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    }

    return interpreter->output(0);
}

uint8_t* DepthEstimationT::decode_inference(TfLiteTensor* output_tensor) {
    if (!this->interpreter) return nullptr;

    // Fixed dimensions (match model output)
    int width = 48; 
    int height = 48;
    int total_pixels = width * height;

    // Allocate buffer
    uint8_t* vis_buffer = (uint8_t*)malloc(total_pixels * sizeof(uint8_t));
    if (!vis_buffer){
        log("Memory problem!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    } 

    int8_t* src = output_tensor->data.int8;
    uint8_t* dst = vis_buffer;
    size_t i = 0;

    // Vectorized conversion: int8 [-128, 127] -> uint8 [0, 255]
    uint32_t* src32 = (uint32_t*)src;
    uint32_t* dst32 = (uint32_t*)dst;

    for (; i <= total_pixels - 4; i += 4) {
        *dst32++ = (*src32++) ^ 0x80808080;
    }

    // Process remaining pixels (if the amountness is not divisible per 4)
    dst += i;
    src += i;
    for (; i < total_pixels; i++) {
        *dst++ = (uint8_t)(*src++ ^ 0x80);
    }

    return vis_buffer;
}