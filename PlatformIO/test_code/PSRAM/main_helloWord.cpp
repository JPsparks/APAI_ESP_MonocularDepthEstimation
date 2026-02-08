#include <Arduino.h>

void setup() {
  Serial.begin(115200);

  delay(5000);
  bool a = psramInit();
  // delay(5000);
  
  if (a) {
      Serial.println("PSRAM OK!");
  } else {
      Serial.println("PSRAM ERROR!");
  }
  
  Serial.println("Modello pronto!");
}

void loop() {
  Serial.println("Sto bene!");
  delay(500);
}
