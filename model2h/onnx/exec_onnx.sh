#!/bin/bash

# CONFIGURATION LOAD

# Try to load config from parent directory (orchestrator style) or current (standalone)
if [ -f "../config.sh" ]; then
    source "../config.sh"
elif [ -f "./config.sh" ]; then
    source "./config.sh"
else
    # Warning only, assuming vars might be already exported
    echo "Warning: config.sh not found. Relying on existing environment variables."
fi


# EXECUTION


echo ""
echo "### Executing ONNX conversion (exploiting ONNX abstraction) #########"
echo "Model Name:      $MODEL_NAME"
echo "ONNX File:       $ONNX_FILE"
echo "Output Target:   $TFLITE_FILE"
echo ""

# Execute Python script
python onnx_to_tflite_4ch_pipeline.py

if [ $? -eq 0 ]; then
    echo "ONNX pipeline finished successfully."
else
    echo "Error in ONNX pipeline."
    exit 1
fi