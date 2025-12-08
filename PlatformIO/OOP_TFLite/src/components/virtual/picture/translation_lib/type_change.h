#ifndef __PW_APAI_STATIC_T_CHANGER_H__
#define __PW_APAI_STATIC_T_CHANGER_H__

#include "../picture.h"


#include "Arduino.h"
#include "esp_jpg_decode.h"

#define TYPECHANGER_LOG_PERMISSION TYPECHANGER_SCALERS_LOG_CONFIG_PERMISSION


Picture* change_byJSONtoRGB888(Picture* by);



#endif