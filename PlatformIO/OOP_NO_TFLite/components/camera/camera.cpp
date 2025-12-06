#include "camera.h"

Camera::Camera(int config){
  this->config = config; //dummy example to evaluate eventually witch feature let me modifiable by extern source
}

int Camera::cameraSetup(void) {

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  
  config.frame_size = FRAMESIZE_240X240; // FRAMESIZE_UXGA;   // ---> FRAMESIZE_240X240
  config.pixel_format = PIXFORMAT_JPEG; //PIXFORMAT_RGB888;   //PIXFORMAT_JPEG; // ---> RGB888    // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  // for larger pre-allocated frame buffer.
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    // Limit the frame size when PSRAM is not available
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return 0;
  }

  sensor_t * s = esp_camera_sensor_get();

  // initial sensors are flipped vertically and colors are a bit saturated
  s->set_vflip(s, 1); // flip it back
  s->set_brightness(s, 1); // up the brightness just a bit
  s->set_saturation(s, 0); // lower the saturation

  Serial.println("Camera configuration complete!");
  return 1;
}

camera_fb_t* Camera::clone_picture(camera_fb_t* to_clone) {
  if (to_clone == nullptr || to_clone->buf == nullptr || to_clone->len == 0) {
        return nullptr;
  }
  
  camera_fb_t* cloned = (camera_fb_t*) malloc(sizeof(camera_fb_t));
  if (cloned == nullptr) {
      return nullptr;
  }

  cloned->buf = (uint8_t*) malloc(to_clone->len);
  if (cloned->buf == NULL) {
      free(cloned);
      return nullptr;
  }

  //copy of the picture itself
  memcpy(cloned->buf, to_clone->buf, to_clone->len);

  //copy of its metadata (lenghts, size, format, etc)
  cloned->len = to_clone->len;
  cloned->width = to_clone->width;
  cloned->height = to_clone->height;
  cloned->format = to_clone->format;
  cloned->timestamp = to_clone->timestamp;

  return cloned;
}

bool Camera::free_picture(camera_fb_t* to_free){
  if (to_free == NULL) {
    return false;
  }
  if (to_free->buf != NULL) {
      free(to_free->buf);
  }
  free(to_free);
  return true;
}

bool Camera::free_my_buffer(void) {
  return this->free_picture(this->last_pick_fb);
}

bool Camera::free_cam_buffer(void){
  esp_camera_fb_return(this->cam_buffer);
  return true;
}

bool Camera::take_picture(void){
  this->free_cam_buffer();
  this->free_my_buffer();

  this->cam_buffer = esp_camera_fb_get();
  this->last_pick_fb = this->clone_picture(this->cam_buffer);

  return true;
}

camera_fb_t* Camera::get_picture(void){
  return this->last_pick_fb;
}


