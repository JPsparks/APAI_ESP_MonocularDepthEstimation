#ifndef __PW_APAI_CAM_H
#define __PW_APAI_CAM_H

// #include "Task.h"
#include <Arduino.h>
#define CAMERA_MODEL_ESP32S3_EYE

#include "esp_camera.h"
#include "camera_pins.h"


class Camera {
  public:
    Camera(int config);
    int cameraSetup(void);
    bool take_picture(void);
    camera_fb_t* get_picture(void);

    bool free_my_buffer(void);
    bool free_cam_buffer(void);
    

  private:
    camera_fb_t* clone_picture(camera_fb_t* to_clone);
    bool free_picture(camera_fb_t* to_free);
    
    camera_fb_t* cam_buffer = nullptr;
    camera_fb_t* last_pick_fb = nullptr;
    int config;


};



#endif 