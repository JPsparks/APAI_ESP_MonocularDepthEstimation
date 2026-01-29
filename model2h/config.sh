#!/bin/bash

# ==============================================================================
# CENTRAL CONFIGURATION
# ==============================================================================

# --- Directory Paths ---
export RAW_DATASET_DIR="${RAW_DATASET_DIR:-./calibration/raw_calibration_dataset}"
export READY_DATASET_DIR="${READY_DATASET_DIR:-../calibration/ready_dataset}"

# --- Script Paths ---
export SCRIPT_ONNX_DIR="${SCRIPT_ONNX_DIR:-./onnx}"
export SCRIPT_TORCH_DIR="${SCRIPT_TORCH_DIR:-./torch}"
export SCRIPT_CALIB_DIR="${SCRIPT_CALIB_DIR:-./calibration}"

# --- Filenames & Models ---
# export MY_TORCH_WEIGHTS_NAME="${SCRIPT_TORCH_DIR}/upydnet_weights"
export MY_TORCH_WEIGHTS_NAME="upydnet_weights"
export TORCH_WEIGHTS="${TORCH_WEIGHTS:-${MY_TORCH_WEIGHTS_NAME}.pth}"   # Source PyTorch weights


MY_MODEL_NAME="uPyD_Net"

export MODEL_NAME="${MODEL_NAME:-${MY_MODEL_NAME}}"
export ONNX_FILE="${ONNX_FILE:-${MODEL_NAME}.onnx}"            # Source ONNX filename

# --- Final results ---
export TFLITE_FILE="${TFLITE_FILE:-${MODEL_NAME}.tflite}"      # Target TFLite filename
export HEADER_FILE="${HEADER_FILE:-${MODEL_NAME}.h}"           # Target C Header filename
export FINAL_OUTPUT_DIR="${FINAL_OUTPUT_DIR:-./result_to_move}"

# --- ESP32 / Hardware Specifics ---
export IMG_HEIGHT="${IMG_HEIGHT:-48}"
export IMG_WIDTH="${IMG_WIDTH:-48}"
export INPUT_CHANNELS="${INPUT_CHANNELS:-3}"


#//just var convetions
export MODEL_NAME_PTT="actModel_ptr"
export MODEL_NAME_LEN="actModel_len"