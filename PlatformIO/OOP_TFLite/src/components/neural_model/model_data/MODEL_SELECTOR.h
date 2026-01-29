#ifndef MODEL_SELECTOR_H
#define MODEL_SELECTOR_H

// #define __PW_APAI_MODEL_DEV_H__ 
// #define __PW_APAI_MODEL_2_H__
#define __PW_APAI_MODEL_FIN_O_H__
// #define __PW_APAI_FIN_T_H__

#endif


#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "MODEL_SELECTOR.h"

#if defined(__PW_APAI_MODEL_DEV_H__)
    #include "depth_estimation.h"

#elif defined(__PW_APAI_MODEL_2_H__)
    #include "model_int8.h"

#elif defined(__PW_APAI_MODEL_FIN_O_H__)
    #include "uPyD_Net.h"

#elif defined(__PW_APAI_FIN_T_H__)
    #include "uPyD_NetT.h"


#else
    #error "At least a model should be defined!"
#endif


#endif 