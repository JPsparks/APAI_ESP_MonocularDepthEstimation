#!/bin/bash

cd torch
# python torch/get_siutable_dataset.py
python get_siutable_dataset.py
python convert_to_tflite.py
