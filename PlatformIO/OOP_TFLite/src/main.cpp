
#include "utility/logger.h"

#include "components/camera/camera.h"
#include "components/board_led/board_led.h"
#include "components/sd/file_manager.h"

#include "components/virtual/picture/picture.h"
#include "components/virtual/picture/translation_lib/type_change.h"
#include "components/virtual/picture/translation_lib/pickers.h"

// #include "components/neural_model/depth_estimation.h"
// #include "components/neural_model/depth_estimation_4ch.h"
#include "components/neural_model/local_model.h"

#define BUTTON_PIN  0

Camera* cam = nullptr;                // object for handling camera sensor
BoardLed* b_led = nullptr;            // led of the ESP32-s3 model
FileManager* fman = nullptr;          // file_manager
LocalModel* myModel = nullptr;        // model wrapped into a class

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

#if defined(USING_TORCH)
    #include "components/neural_model/depth_estimation_t.h"
    #define MODEL_CONSTRUCTOR new DepthEstimationT()
    

#elif defined(USING_ONNX)
    #include "components/neural_model/depth_estimation_o.h"
    #define MODEL_CONSTRUCTOR new DepthEstimationO()

#else
    #error "Define the path you had choosen"
#endif




const char* base_dir = DIR_PHOTOS;

void setup() {
  Serial.begin(115200);
  // Serial.setDebugOutput(false);
  Serial.println();

  if (psramInit()) {
    default_log("Inizialized PSRAM", MAIN_LOG_PERMISSION);
  } else {
    log("PSRAM Fallita!", ERROR);
    while(1); //kill here the programm: this is a crucial error, because neural model need it
  }

  //create the model
  myModel = MODEL_CONSTRUCTOR;
  bool successOp = myModel->defineLocalModel();

  if (!successOp) {
    log("Model is not instantiate!", WARNING);
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  b_led = new BoardLed();

  b_led->ledInit();
  
  int j;
  successOp = fman->sdmmcInit();
  if (!successOp){
    log("The SD is not plugged!", WARNING);
    for (j = 0; j < 3; j++) {
      b_led->ledSetColor(COLOR_ERROR); // Red = Error 
      delay(300);
      b_led->ledSetColor(COLOR_READY); 
    }

  }

  // removeDir(SD_MMC, "/camera");
  // fman->listDir(base_dir, 0);
  // fman->removeDir(base_dir, true);
  fman->createDir(base_dir); 
  fman->listDir(base_dir, 0);

  cam = new Camera();
  bool res = cam->cameraSetup();
  if (res) {
    b_led->ledSetColor(COLOR_READY); // Green = Ready
  } else {
    b_led->ledSetColor(COLOR_ERROR);
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

    Picture* local_pic = nullptr;
    Picture* to_save   = nullptr;
    Picture* zipped    = nullptr;

    
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(20);          // this is needed to handle button bouncing effect

        if (digitalRead(BUTTON_PIN) == LOW) {
  
          // ############## CAPTURING PHOTO ##########
          default_log("-----> Getting picture: ", MAIN_LOG_PERMISSION); //default_log("-----> Cattura immagine: ");//Serial.print();
          #ifdef TIME_COUNT
          init = millis();
          #endif

          b_led->ledSetColor(COLOR_CAPTURE); // Blue = getting photo (before ws2812SetColor(3))
          
          while(digitalRead(BUTTON_PIN) == LOW); //wait user release the button
          
          cam->take_picture();
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

          if (!success){
            delay(1000);
          } else {

            // ############## HANDLING PHOTO ##########
            default_log("-----> Converting by esp_lib into prj class: ", MAIN_LOG_PERMISSION);
            #ifdef TIME_COUNT
            init = millis();
            #endif

            local_pic = new Picture(tmp->buf, tmp->len, tmp->width, tmp->height, JPEG, true);
            #ifdef TIME_COUNT
            end_time(init, "above", &amount);
            #endif

            //~~~
            default_log("-----> By JSON to RGB888: ", MAIN_LOG_PERMISSION);
            #ifdef TIME_COUNT
            init = millis();
            #endif

            if (local_pic->get_format() == JPEG) {
              to_save = change_byJPEGtoRGB888(local_pic);
            } else {
              to_save = local_pic; // in case camera is able to handle directly an RGB format
            }
            
            #ifdef TIME_COUNT
            end_time(init, "above", &amount);
            #endif

            //~~~
            default_log("-----> Picking pixel to adapt 48x48 format: ", MAIN_LOG_PERMISSION);
            #ifdef TIME_COUNT
            init = millis();
            #endif

            zipped = pick_by240to48(to_save, false);

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
                  
              if (photo_index != -1) {
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
            if (success_conversion) {

              b_led->ledSetColor(COLOR_WAIT);
              default_log("-----> Inference: ", MAIN_LOG_PERMISSION);

              #ifdef TIME_COUNT
              init = millis();
              #endif
              
              uint32_t before = CPU_GET_CYCLECOUNTER();
              TfLiteTensor* output = myModel->inference(zipped->get_raw_data());
              uint32_t after = CPU_GET_CYCLECOUNTER();
              uint32_t amount_cycles = after - before;
              
              Serial.print("\n----------------------------------------------\n");
              Serial.printf(" ----> Amount cycles needed: %u <---- \n", amount_cycles);
              Serial.print("----------------------------------------------\n");

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

            // ============================================================
            //  PULIZIA MEMORIA (CRITICO PER EVITARE CRASH)
            // ============================================================
            default_log("Cleaning up memory...", MAIN_LOG_PERMISSION);
            if (local_pic) delete local_pic;
            if (to_save) delete to_save;
            if (zipped) delete zipped;
            
            // Reset pointers for safety
            local_pic = nullptr;
            to_save = nullptr;
            zipped = nullptr;
            
            // Nota: 'tmp' non va cancellato qui perché è gestito internamente 
            // dalla tua classe Camera (last_pick_fb viene riciclato).
          }
        }
    }
}

