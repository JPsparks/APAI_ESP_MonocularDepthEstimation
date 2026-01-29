#include "depth_estimation_o.h"
#include <math.h> 

DepthEstimationO::DepthEstimationO() {
    // Heuristic allocation for Tensor Arena (might need tuning)
    this->kTensorArenaSize = MODEL_NAME_LEN * 4;
}

TfLiteTensor* DepthEstimationO::inference(uint8_t* input_data) {
    if (!this->interpreter) {
        log("Interpreter is missing!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    }
    
    // --- Access Input Tensor ---
    TfLiteTensor* input_tensor = this->interpreter->input(0);

    float scale = input_tensor->params.scale;
    int32_t zero_point = input_tensor->params.zero_point;

    // Safety checks: use defaults if metadata is missing
    if (scale == 0.0f) { scale = 1.0f; zero_point = 0; }
    
    int8_t* dst = input_tensor->data.int8;
    const uint8_t* src = input_data; // Assuming this is still RGB (3 channels)

    // Pre-calculate multiplier to avoid slow division in loop
    // Formula: (pixel_float / 255.0) / scale + zero_point
    float effective_scale_inv = 1.0f / (255.0f * scale);

    // --- 4-CHANNEL ADAPTATION LOOP ---
    // The model expects 48x48x4, but we have 48x48x3 input.
    // We must manually insert the 4th byte (padding) for every pixel.
    
    int num_pixels = 48 * 48; // O input_tensor->dims->data[1] * input_tensor->dims->data[2]
    int src_idx = 0;
    int dst_idx = 0;

    for (int i = 0; i < num_pixels; i++) {
        // Process R, G, B one at a time
        for (int c = 0; c < 3; c++) {
            float val = (float)src[src_idx++] * effective_scale_inv + zero_point;
            
            // Manual quantization with clamp
            int32_t val_q = (int32_t)val; 
            if (val_q < -128) val_q = -128;
            else if (val_q > 127) val_q = 127;
            
            dst[dst_idx++] = (int8_t)val_q;
        }

        // --- 4th CHANNEL (DUMMY) ---
        // Insert 0 (or zero_point) as padding. 
        // Since weights for this channel are 0, the value does not affect the result.
        dst[dst_idx++] = (int8_t)zero_point; 
    }

    // --- Run Inference ---
    if (interpreter->Invoke() != kTfLiteOk) {
        log("Error along inference!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    }

    return interpreter->output(0);
}

// La funzione di decode rimane identica perché l'output è sempre 1 canale (Depth Map)
uint8_t* DepthEstimationO::decode_inference(TfLiteTensor* output_tensor) {
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

    // Check 4-byte alignment for safety before casting to uint32
    if (((uintptr_t)src & 0x3) == 0 && ((uintptr_t)dst & 0x3) == 0) {
        uint32_t* src32 = (uint32_t*)src;
        uint32_t* dst32 = (uint32_t*)dst;

        for (; i <= total_pixels - 4; i += 4) {
            *dst32++ = (*src32++) ^ 0x80808080;
        }
        dst = (uint8_t*)dst32;
        src = (int8_t*)src32;
    }

    // Process remaining pixels
    for (; i < total_pixels; i++) {
        *dst++ = (uint8_t)(*src++ ^ 0x80);
    }

    return vis_buffer;
}