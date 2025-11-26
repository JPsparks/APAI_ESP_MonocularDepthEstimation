// model_int8.h
#ifndef MODEL_INT8_H
#define MODEL_INT8_H

#include <esp_attr.h> // Per la macro DRAM_ATTR (anche se usiamo la Flash qui)

// In model_int8.h o nel file principale
extern const unsigned char model_int8_tflite[];
// Opzionale, se hai anche la lunghezza
const unsigned int model_int8_tflite_len = 135472;

#endif