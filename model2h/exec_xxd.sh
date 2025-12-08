#!/bin/bash

xxd -i uPyD-Net.tflite > model_int8.h

if [ "$1" == "M" ]; then
    mv model_int8.h ../PlatformIO/OOP_TFLite/src/components/neural_model/model_data/new_model.h