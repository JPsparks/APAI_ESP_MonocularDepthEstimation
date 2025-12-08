
#include "depth_estimation.h"

DepthEstimation::DepthEstimation() {
    // Euristica per l'Arena: Dimensione modello + Buffer Tensori temporanei.
    // Se crasha, aumenta questo moltiplicatore o usa il valore esatto calcolato dal profiler.
    // 500KB (come nel tuo esempio setup) è un buon punto di partenza per immagini 48x48
    this->kTensorArenaSize = uPyD_Net_tflite_len * 4;  //for now this is kind euristic
}

TfLiteTensor* DepthEstimation::inference(uint8_t* input_data) {
    if (!this->interpreter){
        log("Interpreter is missing!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    }
    TfLiteTensor* input_tensor = this->interpreter->input(0);

    // Quantizzation parameters, taken by model itself
    float scale = input_tensor->params.scale;
    int32_t zero_point = input_tensor->params.zero_point;

    if (scale == 0.0f) {
        // Use anyway safe dafault values
        scale = 1.0f;
        zero_point = 0;
    }

    for (int i = 0; i < input_tensor->bytes; i++) {
        // Normalization
        float pixel_normalized = (((float)input_data[i]) / 255.0f);

        // Quantizite: q = (real / scale) + Z
        int32_t val_quantized = (pixel_normalized / scale) + zero_point;
        // TODO (maybe): add 0.5f to round to nearest integer

        // Clamping
        if (val_quantized < -128) val_quantized = -128;
        if (val_quantized > 127)  val_quantized = 127;

        input_tensor->data.int8[i] = (int8_t)val_quantized;
    }

    if (interpreter->Invoke() != kTfLiteOk) {
        log("Error along inference!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    }

    return interpreter->output(0);
    
}

uint8_t* DepthEstimation::decode_inference(TfLiteTensor* output_tensor) {
    if (!this->interpreter) return nullptr;
    
    int width = 48; // TODO: leggi da output_tensor->dims->data[2]
    int height = 48;
    int total_pixels = width * height;

    uint8_t* vis_buffer = (uint8_t*)malloc(total_pixels * sizeof(uint8_t));
    if (!vis_buffer){
        log("Memory problem!", ERROR, LOCALMODEL_LOG_PERMISSION);
        return nullptr;
    } 

    // De-quantization parameter (optional, but usefull to show)
    // To visualize a depth map, it is sufficent below:
    // int8 [-128, 127] -> uint8 [0, 255]
    
    for(int i=0; i<total_pixels; i++) {
        int8_t out_val = output_tensor->data.int8[i];
        
        int vis_val = (int)out_val + 128; 
        
        // Clamping
        if (vis_val < 0) vis_val = 0;
        if (vis_val > 255) vis_val = 255;
        
        vis_buffer[i] = (uint8_t)vis_val;
    }

    return vis_buffer;
}
