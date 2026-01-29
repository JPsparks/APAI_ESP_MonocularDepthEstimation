#!/bin/bash

# Source the configuration file
if [ -f "./config.sh" ]; then
    source "./config.sh"
else
    echo "'config.sh' not found. Please ensure it exists in the same directory."
    exit 1
fi


# HELPER FUNCTION

show_help() {
    echo "Usage: ./convert.sh [MODE] [OPTIONS]"
    echo ""
    echo "Orchestrates the conversion pipeline from PyTorch/ONNX to TFLite and C Header."
    echo ""
    echo "MODES (Mandatory, chose one):"
    echo "  O                 Execute the ONNX conversion pipeline."
    echo "  T                 Execute the Torch conversion pipeline."
    echo ""
    echo "OPTIONS:"
    echo "  --no-dt-generation   Skip the dataset preparation/calibration step."
    echo "  --no-h-conv          Skip the final C Header (.h) generation step."
    echo "  -h, --help           Show this help message and exit."
    echo ""
    echo "ENVIRONMENT VARIABLES (Override defaults in config.sh):"
    echo "  MODEL_NAME        Name of the model (Default: $MODEL_NAME)"
    echo "  TFLITE_FILE       Output TFLite filename (Default: $TFLITE_FILE)"
    echo "  HEADER_FILE       Output Header filename (Default: $HEADER_FILE)"
    echo ""
    echo "Examples:"
    echo "  ./convert.sh O"
    echo "  ./convert.sh T --no-dt-generation"
    echo ""
}

# ==============================================================================
# ARGUMENT PARSING
# ==============================================================================

MODE=""
SKIP_DATASET=false
SKIP_XXD=false

# Check if no arguments provided
if [ $# -eq 0 ]; then
    show_help
    exit 1
fi

while [[ $# -gt 0 ]]; do
  case $1 in
    -h|--help)
      show_help
      exit 0
      ;;
    --no-dt-generation)
      SKIP_DATASET=true
      shift
      ;;
    --no-h-conv)
      SKIP_XXD=true
      shift
      ;;
    O|T)
      MODE=$1
      shift
      ;;
    *)
      echo "Error: Unknown argument '$1'"
      echo "Try './convert.sh --help' for usage information."
      exit 1
      ;;
  esac
done

# Validate Mode
if [ -z "$MODE" ]; then
    echo "You must specify a mode ('O' for ONNX or 'T' for Torch)."
    exit 1
fi

# Stop execution on any error from now on
set -e 




### DATASET PREPARATION ###

if [ "$SKIP_DATASET" = false ]; then
    echo "# [1/3] Preparing Dataset... ##  #########"
    
    # Use pushd/popd for safer directory navigation
    pushd "$SCRIPT_CALIB_DIR" > /dev/null
    
    # Run the python script. It will inherit exported variables from config.sh
    python get_suitable_dataset.py
    
    popd > /dev/null
    echo "OK - Dataset prepared."
else
    echo "# [1/3] Skipping dataset preparation (--no-dt-generation active) ##  #########"
fi

echo ""

### MODEL CONVERSION (O or T) ###

echo "# [2/3] Running Conversion Pipeline ($MODE)... ##  #########"

if [ "$MODE" == "O" ]; then
    echo " -> Executing ONNX pipeline..."
    pushd "$SCRIPT_ONNX_DIR" > /dev/null
    ./exec_onnx.sh
    popd > /dev/null

elif [ "$MODE" == "T" ]; then
    echo " -> Executing Torch pipeline..."
    pushd "$SCRIPT_TORCH_DIR" > /dev/null
    ./exec_torch.sh
    popd > /dev/null
fi

echo "OK - Conversion finished successfully. "
echo ""

### HEADER GENERATION (XXD) ###


if [ "$SKIP_XXD" = false ]; then
    echo "# [3/3] Generating C Header (.h)... ##  #########"
    
    # Check if the TFLite file exists before attempting conversion
    # Note: We assume previous scripts saved it to root or specific folders.
    # We try to locate it if not found in current dir.
    if [ ! -f "$TFLITE_FILE" ]; then
        FOUND_TFLITE=$(find . -name "$TFLITE_FILE" | head -n 1)
        if [ -n "$FOUND_TFLITE" ]; then
            echo " -> Found TFLite at: $FOUND_TFLITE"
            TFLITE_FILE="$FOUND_TFLITE"
        else
            echo "Error: File '$TFLITE_FILE' not found. Cannot generate header."
            exit 1
        fi
    fi

    echo " -> Converting $TFLITE_FILE to $HEADER_FILE..."
    
    # Check if exec_xxd.sh exists, otherwise run command directly
    if [ -f "./exec_xxd.sh" ]; then
        ./exec_xxd.sh "$TFLITE_FILE" "$HEADER_FILE"
    else
        # Fallback if the helper script is missing
        xxd -i "$TFLITE_FILE" > "$HEADER_FILE"
        # Optional: Add const (sed command depends on OS, omitting for safety here)
    fi
    
    # echo "Header generated: $HEADER_FILE"
else
    echo "# [3/3] Skipping Header generation (--no-h-conv active) ##  #########"
fi