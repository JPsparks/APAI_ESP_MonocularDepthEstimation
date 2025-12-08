#ifndef __PW_APAI_DEPTH_ESTIMATION_H__
#define __PW_APAI_DEPTH_ESTIMATION_H__

#include "local_model.h"
#include "model_data/depth_estimation.h"



class DepthEstimation : public LocalModel
{
protected:
    
    const unsigned char* getModelINT8() override {
        return uPyD_Net_tflite;
    };
    // int kTensorArenaSize;           // by LocalModel

public:
    DepthEstimation();
    ~DepthEstimation() {
        // Il distruttore base pulirà tutto
    }
    TfLiteTensor* inference(uint8_t* input_data);
    uint8_t* decode_inference(TfLiteTensor* output_tensor);
    
};


#endif