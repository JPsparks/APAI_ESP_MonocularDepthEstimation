#ifndef __PW_APAI_CAM_H
#define __PW_APAI_CAM_H

// #include "Task.h"
#include <Arduino.h>
#include "../../utility/logger.h"
#include "../../config.h"

#define CAMERA_LOG_PERMISSION CAMERA_LOG_CONFIG_PERMISSION

#define CAMERA_MODEL_ESP32S3_EYE //FUNDAMENTAL TO CHANGE if another board respect to esp32s3 is used

#include "esp_camera.h"
#include "camera_pins.h"




class Camera {
  public:
    Camera(); //notice: if in future is needed to modify some parameters, it's a good idea modify in a suitable way the constructor
    bool cameraSetup(void);  //or this method (but better the constructor)
    bool take_picture(void);
    camera_fb_t* get_picture(void);

    //method to free buffer of this class
    bool free_my_buffer(void); // the one inside this class
    bool free_cam_buffer(void); // the one inside the imported (and used) lib
    

  private:
    // common procedure of last two functions
    bool free_picture(camera_fb_t* to_free);
    // utility function to create copy of buffer
    camera_fb_t* clone_picture(camera_fb_t* to_clone);
    
    // state
    camera_fb_t* cam_buffer = nullptr;
    camera_fb_t* last_pick_fb = nullptr;
    int config;


};



#endif 