


// specify here the root of photos
#define DIR_PHOTOS "/lab20251212_2"

// enable / disable cycle count
#define TIME_COUNT


// just var convetions
#define MODEL_NAME_PTT actModelPtr
#define MODEL_NAME_LEN actModelLen

// define with kind of model used
// #define USING_TORCH
#define USING_ONNX


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
