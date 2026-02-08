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

# NOTICE: if runned not in "conver.sh" context, 
# could be better redefine some vars

# EXECUTION

echo ""
echo "### Executing Torch conversion (PyTorch -> Keras -> TFLite) #########"
echo "Model Name:      $MODEL_NAME"
echo "Weights File:    $TORCH_WEIGHTS"
echo "Output Target:   $TFLITE_FILE"
echo ""
#these plots are usefull only too be sure that these vars have expected values

# Execute Python script
python convert_pt_to_keras_to_tflite.py

if [ $? -eq 0 ]; then
    echo "Torch pipeline finished successfully."
else
    echo "Error in Torch pipeline."
    exit 1
fi