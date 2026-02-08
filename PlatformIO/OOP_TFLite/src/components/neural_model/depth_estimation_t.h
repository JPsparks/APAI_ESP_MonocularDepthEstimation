#ifndef __PW_APAI_DEPTH_ESTIMATION_H__
#define __PW_APAI_DEPTH_ESTIMATION_H__

#include "local_model.h"
#include "model_data/MODEL_LOADER.h"

class DepthEstimationT : public LocalModel
{
protected:
    
    const unsigned char* getModelINT8() override {
        return MODEL_NAME_PTT;
    };
    // int kTensorArenaSize;           // REMINDER: This come by LocalModel

public:
    DepthEstimationT();
    ~DepthEstimationT() {
        // enought capable to clean all, it is not required to define anything
    }
    TfLiteTensor* inference(uint8_t* input_data) override;             // here is needed an overriding of these methods
    uint8_t* decode_inference(TfLiteTensor* output_tensor) override;   // here is needed an overriding of these methods
    
};


#endif