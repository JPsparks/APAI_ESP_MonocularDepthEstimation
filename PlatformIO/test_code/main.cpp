/**********************************************************************
  Filename    : Camera and SDcard
  Description : Use the onboard buttons to take photo and save them to an SD card.
  Auther      : www.freenove.com
  Modification: 2022/11/02
**********************************************************************/
#include "esp_camera.h"
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"
// #include "ws2812.h"
#include "sd_read_write.h"



#include <FastLED.h>

// Configurazione LED WS2812
#define LED_PIN      48  // Modifica con il pin corretto
#define NUM_LEDS     1   // Numero di LED (di solito 1 per indicatore di stato)
#define LED_TYPE     WS2812B
#define COLOR_ORDER  GRB

CRGB leds[NUM_LEDS];

// Definizione colori
#define COLOR_OFF      CRGB::Black
#define COLOR_ERROR    CRGB::Red
#define COLOR_READY    CRGB::Green
#define COLOR_CAPTURE  CRGB::Blue

void ledSetColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
}

void ledInit() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(50); // Luminosità 0-255
  ledSetColor(COLOR_OFF);
}

// Funzione opzionale per lampeggio
void ledBlink(CRGB color, int times = 3, int delayMs = 200) {
  for(int i = 0; i < times; i++) {
    ledSetColor(color);
    delay(delayMs);
    ledSetColor(COLOR_OFF);
    delay(delayMs);
  }
}

#define BUTTON_PIN  0

int cameraSetup(void);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

//   ws2812Init();
  ledInit();
  sdmmcInit();

  //removeDir(SD_MMC, "/camera");
  createDir(SD_MMC, "/camera");
  listDir(SD_MMC, "/camera", 0);

  if(cameraSetup()==1){
    ledSetColor(COLOR_READY); // Verde = pronta ws2812SetColor(2);
  }
  else{
    ledSetColor(COLOR_ERROR); // Rosso = errore ws2812SetColor(1);
    return;
  }
}

void loop() {
    unsigned long init;
    unsigned long fin;
    // unsigned long tempoEsecuzione;
    if(digitalRead(BUTTON_PIN)==LOW){
        delay(20);

        if(digitalRead(BUTTON_PIN)==LOW){
            init = millis();
            ledSetColor(COLOR_CAPTURE); // Blu = scatto in corso (vecchio ws2812SetColor(3);)
            while(digitalRead(BUTTON_PIN)==LOW);

            camera_fb_t * fb = NULL;
            
            fb = esp_camera_fb_get();
            fin = millis();
            Serial.print("-----> ");
            Serial.println(fin - init);

            if (fb != NULL) {
                init = millis();
                int photo_index = readFileNum(SD_MMC, "/camera");
                if(photo_index!=-1) {
                    String path = "/camera/" + String(photo_index) +".jpg";
                    writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);
                    // Lampeggio verde per conferma salvataggio
                    ledBlink(COLOR_READY, 2, 100);
                }
                esp_camera_fb_return(fb);
                fin = millis();
                Serial.print("-----> ");
                Serial.println(fin - init);
            }
            else {
                Serial.println("Camera capture failed.");
                // Lampeggio rosso per errore
                ledBlink(COLOR_ERROR, 3, 150);
            }
            // ws2812SetColor(2);
            ledSetColor(COLOR_READY); // Torna a verde = pronta
        }
    }
}

int cameraSetup(void) {
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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
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
