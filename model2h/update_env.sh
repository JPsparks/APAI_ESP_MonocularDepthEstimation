#!/bin/bash

echo "--------------------------------------------------------"
echo "Environment Setup Recommended"
echo "Be sure that you have created an environment (suggested python version = 3.10) and have enabled it"
echo "--------------------------------------------------------"
read -r -p "Proceed? (y/[n]) " confirmation_input

confirmation_input=$(echo "$confirmation_input" | tr '[:upper:]' '[:lower:]')
if [ "$confirmation_input" != "y" ]; then
    echo "Aborting execution as requested."
    exit 0 
fi

echo "Proceeding with execution..."

# --- STEP 1: CLEANUP (The most important part) ---
# We force uninstall onnx multiple times to remove conflicting versions
echo "Cleaning up potential conflicting installations..."
python -m pip uninstall onnx -y
python -m pip uninstall onnx -y
python -m pip uninstall onnx-tf -y
python -m pip uninstall onnx2tf -y

# --- STEP 2: INSTALLATION ---

# 1. Install standard PyTorch (CPU is fine for export)
python -m pip install torch torchvision --index-url https://download.pytorch.org/whl/cpu

# 2. Install TensorFlow (Modern) and tf_keras
python -m pip install --upgrade "tensorflow>=2.15.0"
python -m pip install tf_keras

# 3. Install ONNX (Strict Versioning)
# We use 1.16.1 because it definitely contains the required helper functions
python -m pip install "onnx==1.16.1"
python -m pip install onnxscript
python -m pip install onnx-simplifier

# 4. Install onnx2tf (Golden Version)
# We use 1.22.3 because it avoids the "ai_edge_litert" error on Windows
python -m pip install "onnx2tf==1.22.3"
python -m pip install sng4onnx simple_onnx_processing_tools
python -m pip install --upgrade onnx-graphsurgeon

# 5. Utilities
python -m pip install pillow numpy psutil


echo "--------------------------------------------------------"
echo "VERIFICATION:"
# We verify immediately using the same python interpreter
python -c "import onnx; print(f'ONNX Version: {onnx.__version__}'); import onnx.helper; print(f'Helper Check: {hasattr(onnx.helper, \"float32_to_bfloat16\")}')"
echo "--------------------------------------------------------"