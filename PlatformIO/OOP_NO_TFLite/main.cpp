
#include "components/camera/camera.h"
#include "components/board_led/board_led.h"
#include "components/sd/file_manager.h"

#include "components/virtual/picture/picture.h"
#include "components/virtual/picture/type_change.h"
#include "components/virtual/picture/pickers.h"

#define BUTTON_PIN  0

Camera* cam = nullptr;
BoardLed* b_led = nullptr;
FileManager* fman = nullptr;

const char* base_dir = "/cameraRGB888";

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  b_led = new BoardLed();

  b_led->ledInit();
  fman->sdmmcInit();

  //removeDir(SD_MMC, "/camera");
  fman->createDir(base_dir); 
  fman->listDir(base_dir, 0);

  cam = new Camera(1);
  int res = cam->cameraSetup();
  if (res == 1) {
    b_led->ledSetColor(COLOR_READY); // Green = Ready  (before ws2812SetColor(2))
  } else {
    b_led->ledSetColor(COLOR_ERROR); // Red = Error    (before ws2812SetColor(1))
    return;
  }
}

void loop() {
    unsigned long init;
    unsigned long fin;
    
    if(digitalRead(BUTTON_PIN) == LOW){
        delay(20);

        if(digitalRead(BUTTON_PIN) == LOW){
            init = millis();

            b_led->ledSetColor(COLOR_CAPTURE); // Blue = getting photo scatto (before ws2812SetColor(3))
            while(digitalRead(BUTTON_PIN)==LOW);

            // fb = esp_camera_fb_get();
            camera_fb_t* tmp = nullptr;
            if(cam->take_picture()){
              tmp = cam->get_picture();
            } else {
              Serial.print("ERR");
            }

            fin = millis();
            Serial.print("-----> ");
            Serial.println(fin - init);

            Serial.print("-----> Converting by esp_lib into prj class");
            Picture* local_pic = new Picture(tmp->buf, tmp->len, tmp->width, tmp->height, JPEG, true); // <-- to check 
            Serial.print("-----> By JSON to RGB888");
            Picture* to_save = change_byJSONtoRGB888(local_pic);
            Serial.print("-----> Picking pixel to adapt 48x48 format");
            Picture* zipped = pick_by240to48(to_save, false);

            if ((tmp != nullptr) && (to_save != nullptr)) {

                init = millis();
                int photo_index = fman->readFileNum(base_dir);
                
                if (photo_index!=-1) {
                    String path = String(base_dir) + "/" + String(photo_index) + ".jpg";
                    String path2 = String(base_dir) + "/RGB" + String(photo_index) + ".bmp"; 
                    String path3 = String(base_dir) + "/RGB" + String(photo_index) + "SMALL.bmp";

                    fman->writejpg(path.c_str(), tmp->buf, tmp->len);

                    b_led->ledBlink(COLOR_READY, 2, 100); //show that everything gone well!

                    fman->writebmp(path2.c_str(), to_save->get_raw_data(), to_save->get_width(), to_save->get_height());
                    fman->writebmp(path3.c_str(), zipped->get_raw_data(), zipped->get_width(), zipped->get_height());
                }
                
                fin = millis();
                Serial.print("-----> ");
                Serial.println(fin - init);
            }
            else {
                Serial.println("Camera capture failed.");
                b_led->ledBlink(COLOR_ERROR, 3, 150); // show that something gone wrong
            }
            b_led->ledSetColor(COLOR_READY); // Return back ready
        }
    }
}

