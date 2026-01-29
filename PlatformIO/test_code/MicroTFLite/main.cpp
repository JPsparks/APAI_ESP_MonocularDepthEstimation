#include "model_int8.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

// constexpr int kTensorArenaSize = 10 * 1024;
// constexpr int kTensorArenaSize = 400 * 1024;
constexpr int kTensorArenaSize = 135500;//407000;//405520;//135472;//model_int8_tflite_len;//64 * 1024;
uint8_t tensor_arena[kTensorArenaSize] __attribute__((section(".ext_ram")));

const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;

void setup() {
  Serial.begin(115200);

  Serial.println("A");

  // 1. Carica modello
  model = tflite::GetModel(model_int8_tflite);
  Serial.println("B");
  static tflite::AllOpsResolver resolver;
  Serial.println("C");

  // 2. Crea interprete globale
  interpreter = new tflite::MicroInterpreter(
      model, resolver, tensor_arena, kTensorArenaSize);

  Serial.println("D");

  // 3. Alloca tensori
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("Errore nell'allocazione dei tensori!");
    delay(3000);
    // while (1);
  }

  Serial.println("Modello pronto!");
}

void loop() {
  TfLiteTensor* input_tensor = interpreter->input(0);

  for (int i = 0; i < input_tensor->bytes; i++) {
    input_tensor->data.uint8[i] = random(0, 255);
  }

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Errore durante l'inferenza!");
    return;
  }

  TfLiteTensor* output = interpreter->output(0);

  // Serial.print("Output[0] = ");
  // Serial.println(output->data.uint8[0]);
  Serial.println("Sto bene!");
  delay(500);
}
