#ifndef __PW_APAI_LOCAL_MODEL_H__
#define __PW_APAI_LOCAL_MODEL_H__

#include <Arduino.h>
#include "../../utility/logger.h"
#include "../../config.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_profiler.h"

#define LOCALMODEL_LOG_PERMISSION LOCALMODEL_LOG_CONFIG_PERMISSION

#define MAX_PROFILING_LAYERS 100
//notice: this is overstimed, most of these networks have quite less amount layers

class Esp32CycleProfiler : public tflite::MicroProfiler {
public:
    const char* model_name;

    // comodoty to handle in a simply way all data related to a specific i-th layer, starting by 0
    struct LayerStat {
        const char* tag;
        uint32_t duration;
    };

    LayerStat stats[MAX_PROFILING_LAYERS];
    int event_count = 0;

    Esp32CycleProfiler(const char* name = "Model") : model_name(name) {}

    // --- Custom Memory Management ---
    // TFLite Micro hides/deletes the operator delete. We override it publicy
    // to allow 'delete profiler' and dynamic allocation on ESP32.
    // This is due mainly because it is not a TF stricly need, and ideally this space is
    // allocated once and never free (because ideally we need alwase the model in the background)
    void operator delete(void* p) { free(p); }
    void* operator new(size_t size) { return malloc(size); }
    // ---------------------------------------------

    void Clear() {
        event_count = 0;
    }

    uint32_t BeginEvent(const char* tag) override {
        if (event_count < MAX_PROFILING_LAYERS) {
            stats[event_count].tag = tag;
        }
        return CPU_GET_CYCLECOUNTER();
    }

    void EndEvent(uint32_t start_handle) override {
        if (event_count < MAX_PROFILING_LAYERS) {
            uint32_t end_handle = CPU_GET_CYCLECOUNTER();
            stats[event_count].duration = end_handle - start_handle;
            event_count++;
        }
    }

    void PrintStats() {
        Serial.println("\n--- PROFILING REPORT ---");
        uint32_t total_cycles = 0;
        
        for (int i = 0; i < event_count; i++) {
            Serial.printf("[%s] Layer %02d: %-14s -> %9u cycles\n", 
                          model_name, i, stats[i].tag, stats[i].duration);
            total_cycles += stats[i].duration;
        }
        
        Serial.printf("------------------------\n");
        Serial.printf("TOTAL TENSOR CYCLES: %u  (inside LocalModel)\n", total_cycles);
        // Serial.println("------------------------");
    }
};


class LocalModel
{
private:

protected:
    uint8_t* tensor_arena = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    
    Esp32CycleProfiler* profiler = nullptr;

    virtual const unsigned char* getModelINT8() = 0;

    int kTensorArenaSize;


    // Checks interpreter status and resets the profiler.
    // Usefull for every kind of overriding below function
    bool prepareInference() {
        if (!interpreter) {
            log("Interpreter is missing!", ERROR, LOCALMODEL_LOG_PERMISSION);
            return false;
        }
        if (profiler) {
            profiler->Clear();
        }
        return true;
    }

public:
    
    LocalModel() {}

    virtual ~LocalModel() {
        if (interpreter) delete this->interpreter;
        if (profiler) delete this->profiler;
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

        this->profiler = new Esp32CycleProfiler("Inference");
        
        this->interpreter = new tflite::MicroInterpreter(
            this->model, 
            resolver, 
            this->tensor_arena, 
            this->kTensorArenaSize,
            nullptr,   
            this->profiler
        );  
        
        TfLiteStatus allocate_status = this->interpreter->AllocateTensors();
        if (allocate_status != kTfLiteOk) {
            log("ERROR: AllocateTensors failed!", ERROR, LOCALMODEL_LOG_PERMISSION);
            free(this->tensor_arena);
            log("Arena needed [bytes]: " + String(interpreter->arena_used_bytes()), INFO, true);
            return false;
        }

        log("Arena got at least [bytes]: " + String(interpreter->arena_used_bytes()), INFO, true);
        log("Model uploaded and tensors allocated correctly", INFO, LOCALMODEL_LOG_PERMISSION);
        return true;
    }

    // Public method to print profiling results on demand
    // and not while it is making inference
    // (usefull to get a more precise counting of ms that this process nees)
    void logProfilingResults() {
        if (profiler) {
            profiler->PrintStats();
        }
    }

    // Base inference implementation (optimized for speed/bitwise ops)
    // Child classes should override this if scale/zero_point logic is needed.
    virtual TfLiteTensor* inference(uint8_t* input_data) {
        
        if (!this->prepareInference()) {
            log("Interpreter is missing!", ERROR, LOCALMODEL_LOG_PERMISSION);
            return nullptr;
        }
        
        TfLiteTensor* input_tensor = interpreter->input(0);

        int8_t* dst = input_tensor->data.int8;
        const uint8_t* src = input_data;
        size_t count = input_tensor->bytes;

        size_t i = 0;
        
        uint32_t* dst32 = (uint32_t*)dst;
        const uint32_t* src32 = (const uint32_t*)src;
        const uint32_t mask = 0x80808080; 

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

    // let children class define how to decode the inference if needed
    virtual uint8_t* decode_inference(TfLiteTensor* output_tensor) = 0;
};

#endif