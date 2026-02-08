

/////  CRUCIAL CONFIGS /////////////////////////////////////

// define with kind of model used
// ONE AND ONLY ONE BETWEEN THESE

// #define USING_ONNX
#define USING_TORCH



// define the exact data of the model you whould use
// ONE AND ONLY ONE BETWEEN THESE

// #define __PW_APAI_MODEL_DEV_H__ 
// #define __PW_APAI_MODEL_2_H__
// #define __PW_APAI_MODEL_FIN_O_H__
#define __PW_APAI_FIN_T_H__

// specify here the root of photos
#define DIR_PHOTOS "/lab20251212_2"


/////////////////////////////////

// enable/disable reset of SD every time the board reboot
// #define ALWASE_CLEAN



// enable / disable ms count for each macrostep
#define TIME_COUNT

// function to count performance counter
#if defined(ESP32) 
    #include "esp_cpu.h" 

    // NEVER RESET CCOUNT REGISTER IN ESP32! This could break FreeRTOS.
    #define CPU_RESET_CYCLECOUNTER    /* Do nothing on ESP32, because is harmfull */
    
    // Utility macro to get act "timestamp" of cycles
    #define CPU_GET_CYCLECOUNTER()    ESP.getCycleCount()

// ARM definitions
#elif defined(__arm__)
    #define CPU_RESET_CYCLECOUNTER    do { ARM_DEMCR |= ARM_DEMCR_TRCENA; \
                                           ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA; \
                                           ARM_DWT_CYCCNT = 0; } while(0)
    #define CPU_GET_CYCLECOUNTER()    ARM_DWT_CYCCNT
#else
    #error "Architecture not known"
#endif


// just var convetions
#define MODEL_NAME_PTT actModelPtr
#define MODEL_NAME_LEN actModelLen


// enable log mecanism for debug and generally get more informations
#define DEBUG_LOG

#ifdef DEBUG_LOG

// #define BOARDLED_LOG_CONFIG_PERMISSION              true
#define CAMERA_LOG_CONFIG_PERMISSION                true
#define LOCALMODEL_LOG_CONFIG_PERMISSION            true
#define FILEMAN_LOG_CONFIG_PERMISSION               true

// #define PICTURE_LOG_CONFIG_PERMISSION               true

// #define PICTURE_PICKERS_LOG_CONFIG_PERMISSION       true
// #define PICTURE_SCALERS_LOG_CONFIG_PERMISSION       true
#define TYPECHANGER_SCALERS_LOG_CONFIG_PERMISSION   false

#else

// notice that in this case all log prints are disabled by design

// #define BOARDLED_LOG_CONFIG_PERMISSION              false
#define CAMERA_LOG_CONFIG_PERMISSION                false
#define LOCALMODEL_LOG_CONFIG_PERMISSION            false
#define FILEMAN_LOG_CONFIG_PERMISSION               false

// #define PICTURE_LOG_CONFIG_PERMISSION               false

// #define PICTURE_PICKERS_LOG_CONFIG_PERMISSION       false
// #define PICTURE_SCALERS_LOG_CONFIG_PERMISSION       false
#define TYPECHANGER_SCALERS_LOG_CONFIG_PERMISSION   false

#endif
