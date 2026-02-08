
#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "../../../config.h"

#if defined(__PW_APAI_MODEL_DEV_H__)  //old version, just keep it as inspiration
    #include "depth_estimation.h"

#elif defined(__PW_APAI_MODEL_2_H__) //old version, just keep it as inspiration
    #include "model_int8.h"

#elif defined(__PW_APAI_FIN_O_H__) //should work, but for this model ONNX is not stable
    #include "uPyD_NetO.h"

#elif defined(__PW_APAI_FIN_T_H__)
    #include "uPyD_NetT.h"


#else
    #error "At least a model should be defined!"
#endif


#endif 