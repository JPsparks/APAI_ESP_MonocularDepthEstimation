


#define TIME_COUNT



// #define DEBUG_LOG

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

// #define BOARDLED_LOG_CONFIG_PERMISSION              false
#define CAMERA_LOG_CONFIG_PERMISSION                false
#define LOCALMODEL_LOG_CONFIG_PERMISSION            false
#define FILEMAN_LOG_CONFIG_PERMISSION               false

// #define PICTURE_LOG_CONFIG_PERMISSION               false

// #define PICTURE_PICKERS_LOG_CONFIG_PERMISSION       false
// #define PICTURE_SCALERS_LOG_CONFIG_PERMISSION       false
#define TYPECHANGER_SCALERS_LOG_CONFIG_PERMISSION   false

#endif
