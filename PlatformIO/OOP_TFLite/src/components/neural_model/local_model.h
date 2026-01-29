#ifndef __PW_APAI_LOCAL_MODEL_H__
#define __PW_APAI_LOCAL_MODEL_H__

#include <Arduino.h>
#include "../../utility/logger.h"
#include "../../config.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"

#define LOCALMODEL_LOG_PERMISSION LOCALMODEL_LOG_CONFIG_PERMISSION

class LocalModel
{
private:

protected:
    uint8_t* tensor_arena = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;

    virtual const unsigned char* getModelINT8() = 0;

    int kTensorArenaSize;

public:
    ~LocalModel() {
        if (interpreter) delete this->interpreter;
        // if (resolver) delete this->resolver;
        if (tensor_arena) free(this->tensor_arena);
    }

    bool defineLocalModel() {
        String to_log = "";
        if (this->kTensorArenaSize <= 0) {
            log("ERROR: \"kTensorArenaSize\" not config! Check the builder definition of this model!", ERROR, LOCALMODEL_LOG_PERMISSION);
            return false;
        }

        this->tensor_arena = (uint8_t*)heap_caps_malloc(this->kTensorArenaSize, MALLOC_CAP_SPIRAM);
        if (tensor_arena == NULL) {
            log("ERROR: PSAM (SPIRAM) allocation failed!", ERROR, LOCALMODEL_LOG_PERMISSION);
            return false;
        }
        
        to_log = "PSRAM allocated at 0x" + String((uint32_t)tensor_arena) + " (Size: " + this->kTensorArenaSize + ")";
        default_log(to_log, LOCALMODEL_LOG_PERMISSION);

        this->model = tflite::GetModel(this->getModelINT8());
        
        if (this->model->version() != TFLITE_SCHEMA_VERSION) {
            log("ERROR: version schema of model not supported!", ERROR, LOCALMODEL_LOG_PERMISSION);
            free(this->tensor_arena);
            return false;
        }

        static tflite::AllOpsResolver resolver;

        this->interpreter = new tflite::MicroInterpreter(
            this->model, resolver, this->tensor_arena, this->kTensorArenaSize);  
        
        TfLiteStatus allocate_status = this->interpreter->AllocateTensors();
        if (allocate_status != kTfLiteOk) {
            log("ERROR: AllocateTensors failed!", ERROR, LOCALMODEL_LOG_PERMISSION);
            free(this->tensor_arena);
            // Nice for debug, print the real need of size
            log("Arena needed [bytes]: " + String(interpreter->arena_used_bytes()), INFO, true);
            return false;
        }

        log("Arena got at least [bytes]: " + String(interpreter->arena_used_bytes()), INFO, true);
        log("Model uploaded and tensors allocated correctly", INFO, LOCALMODEL_LOG_PERMISSION);
        return true;
    }


    
    TfLiteTensor* inference(uint8_t* input_data) {
        if (!interpreter) {
            log("Interpreter is missing!", ERROR, LOCALMODEL_LOG_PERMISSION);
            return nullptr;
        } 
        TfLiteTensor* input_tensor = interpreter->input(0);

        int8_t* dst = input_tensor->data.int8;
        const uint8_t* src = input_data;
        size_t count = input_tensor->bytes;

        size_t i = 0;
        
        // Cast to 32-bit pointers for block processing
        uint32_t* dst32 = (uint32_t*)dst;
        const uint32_t* src32 = (const uint32_t*)src;
        
        // Mask to flip MSB on 4 bytes simultaneously (effectively -128)
        const uint32_t mask = 0x80808080; 

        // Main loop: Process 4 bytes/pixels per cycle
        for (; i <= count - 4; i += 4) {
            *dst32++ = (*src32++) ^ mask;
        }

        // Handle remaining bytes (tail)
        dst += i;
        src += i;
        
        for (; i < count; i++) {
            *dst++ = (int8_t)((*src++) ^ 0x80);
        }

        if (interpreter->Invoke() != kTfLiteOk) {
            log("Error along inference!", ERROR, LOCALMODEL_LOG_PERMISSION);
            return nullptr;
        }

        return interpreter->output(0);
    }

    virtual uint8_t* decode_inference(TfLiteTensor* output_tensor) = 0;
};

#endif 