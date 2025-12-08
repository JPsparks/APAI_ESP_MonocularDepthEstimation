
#include "utility/logger.h"

#include "components/camera/camera.h"
#include "components/board_led/board_led.h"
#include "components/sd/file_manager.h"

#include "components/virtual/picture/picture.h"
#include "components/virtual/picture/translation_lib/type_change.h"
#include "components/virtual/picture/translation_lib/pickers.h"

#include "components/neural_model/depth_estimation.h"

#define BUTTON_PIN  0

Camera* cam = nullptr;
BoardLed* b_led = nullptr;
FileManager* fman = nullptr;
DepthEstimation* myModel = nullptr;

#define MAIN_LOG_PERMISSION true

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


const char* base_dir = "/cleaning"; //"/complete";//"/cameraRGB888";

void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(false);
  Serial.println();

  if(psramInit()){
    // Serial.println("PSRAM Inizializzata");
    default_log("Inizialized PSRAM", MAIN_LOG_PERMISSION);
  } else {
    log("PSRAM Fallita!", ERROR);
    while(1); //kill here the programm
  }

  //create the model
  myModel = new DepthEstimation();
  bool successDefinitionModel = myModel->defineLocalModel();
  if (!successDefinitionModel) {
    log("Model is not instantiate!", WARNING);
  }

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

#ifdef TIME_COUNT
unsigned long end_time(unsigned long init, String description, unsigned long* total_time_to_increase){
  unsigned long delta = millis();
  delta -= init;
  String to_log = "Time for " + description + " was: " + String(delta) + "\n";
  log(to_log, WARNING, MAIN_LOG_PERMISSION);

  if (total_time_to_increase) {
    (* total_time_to_increase) += delta;
  }
  return delta;
}
#endif

void loop() {
    #ifdef TIME_COUNT
    unsigned long init;
    unsigned long amount = 0;
    #endif
    
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(20);

        if (digitalRead(BUTTON_PIN) == LOW) {
  
          // ############## CAPTURING PHOTO ##########
          default_log("-----> Getting picture: ", MAIN_LOG_PERMISSION); //default_log("-----> Cattura immagine: ");//Serial.print();
          #ifdef TIME_COUNT
          init = millis();
          #endif

          b_led->ledSetColor(COLOR_CAPTURE); // Blue = getting photo (before ws2812SetColor(3))
          
          while(digitalRead(BUTTON_PIN) == LOW); //wait user release the button
          
          camera_fb_t* tmp = nullptr;
          bool success = cam->take_picture();
          if (success) {
            tmp = cam->get_picture();
          } else {
            log("Error in capturing photo", ERROR);
          }

          #ifdef TIME_COUNT
          end_time(init, "above", &amount);
          #endif
          
          b_led->ledSetColor(COLOR_DEBUG);



          // ############## HANDLING PHOTO ##########
          // Serial.print("-----> Converting by esp_lib into prj class: ");
          default_log("-----> Converting by esp_lib into prj class: ", MAIN_LOG_PERMISSION);
          #ifdef TIME_COUNT
          init = millis();
          #endif

          Picture* local_pic = new Picture(tmp->buf, tmp->len, tmp->width, tmp->height, JPEG, true); // <-- to check 
          #ifdef TIME_COUNT
          end_time(init, "above", &amount);
          #endif



          //~~~
          default_log("-----> By JSON to RGB888: ", MAIN_LOG_PERMISSION);
          #ifdef TIME_COUNT
          init = millis();
          #endif

          Picture* to_save = change_byJSONtoRGB888(local_pic);
          
          #ifdef TIME_COUNT
          end_time(init, "above", &amount);
          #endif



          //~~~
          default_log("-----> Picking pixel to adapt 48x48 format: ", MAIN_LOG_PERMISSION);
          #ifdef TIME_COUNT
          init = millis();
          #endif

          Picture* zipped = pick_by240to48(to_save, false);

          #ifdef TIME_COUNT
          end_time(init, "above", &amount);
          #endif


          bool success_conversion = ((tmp != nullptr) && (to_save != nullptr));
          int photo_index = -1;

          // ############## STORE ACT RESULTS ##########
          if (success_conversion) {
            default_log("-----> Storing: ", MAIN_LOG_PERMISSION);
            #ifdef TIME_COUNT
            init = millis();
            #endif
            photo_index = fman->readFileNum(base_dir);
                
            if (photo_index!=-1) {
              String path = String(base_dir) + "/" + String(photo_index) + ".jpg";
              String path2 = String(base_dir) + "/" + String(photo_index) + "_RGB.bmp"; 
              String path3 = String(base_dir) + "/" + String(photo_index) + "_RGB_SMALL.bmp";

              fman->writejpg(path.c_str(), tmp->buf, tmp->len);

              b_led->ledBlink(COLOR_READY, 2, 100); //show that everything gone well!

              fman->writebmp(path2.c_str(), to_save->get_raw_data(), to_save->get_width(), to_save->get_height(), 3);
              fman->writebmp(path3.c_str(), zipped->get_raw_data(), zipped->get_width(), zipped->get_height(), 3);
            } else {
              log("An error in checking file in dir in file system occour", ERROR, MAIN_LOG_PERMISSION);
            }

          } else {
              log("Camera capture failed.", ERROR, MAIN_LOG_PERMISSION);
              b_led->ledBlink(COLOR_ERROR, 3, 150); // show that something gone wrong
          }

          // ############## APPLY MODEL ##########
          if (success_conversion){

            b_led->ledSetColor(COLOR_WAIT);

            default_log("-----> Inference: ", MAIN_LOG_PERMISSION);
            #ifdef TIME_COUNT
            init = millis();
            #endif
            

            // CYCLECOUNTER;
            uint32_t before = CPU_GET_CYCLECOUNTER();
            TfLiteTensor* output = myModel->inference(zipped->get_raw_data());
            uint32_t after = CPU_GET_CYCLECOUNTER();
            uint32_t amount_cycles = after - before;
            Serial.printf("\n\n ----> Amount cycles needed: %u <---- \n\n", amount_cycles);


            #ifdef TIME_COUNT
            end_time(init, "above", &amount);
            #endif
            

            if (output != nullptr) {
                
              default_log("-----> Decode inference: ", MAIN_LOG_PERMISSION);
              #ifdef TIME_COUNT
              init = millis();
              #endif
              
              uint8_t* decoded = myModel->decode_inference(output);
              
              #ifdef TIME_COUNT
              end_time(init, "above", &amount);
              #endif


              String path4 = String(base_dir) + "/" + String(photo_index) + "depth_map.bmp";
                

              default_log("----->  Storing inference: ", MAIN_LOG_PERMISSION);
              #ifdef TIME_COUNT
              init = millis();
              #endif
              
              fman->writebmp(path4.c_str(), decoded, zipped->get_width(), zipped->get_height(), 1);
              
              #ifdef TIME_COUNT
              end_time(init, "above", &amount);
              #endif
              
            } else {
              log("Error in inference: returned null", ERROR, MAIN_LOG_PERMISSION);
            }
          }

          // Serial.printf("\nTotale: %u\n", amount);
          String final_time_to_log = "Totale: " + String(amount);
          log(final_time_to_log, WARNING, MAIN_LOG_PERMISSION);

          b_led->ledSetColor(COLOR_READY); // Return back ready
        }
    }
}

