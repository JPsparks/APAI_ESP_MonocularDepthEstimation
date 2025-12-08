#!/bin/bash

# ./exec_gain_tflite.sh
# ./exec_xxd.sh



# Check the first command-line argument ($1)
if [ "$1" == "O" ]; then
    # If the first argument is "O", execute exec_onnx.sh
    echo "Argument 'O' detected. Executing ./exec_onnx.sh"
    echo "WARNING: this is a not updated path. Be carefull!"
    cd onnx
    ./exec_onnx.sh
elif [ "$1" == "T" ]; then
    # If the first argument is "T", execute exec_torch.sh (assuming 'torch' meant exec_torch.sh)
    echo "Argument 'T' detected. Executing ./exec_torch.sh"
    cd torch
    ./exec_torch.sh
else
    # If the argument is neither "O" nor "T", print a message and exit.
    # The script will stop here and NOT execute the final ./exec_xxd.sh
    echo "Invalid argument '$1'. Write \"O\" if you choose onnx, \"T\" if otherwise you choos torch path."
    echo "Stopping execution."
    exit 1 # Exit with a successful status (0)
fi

cd ..
# This line is executed ONLY if one of the 'if' or 'elif' branches was executed
# (i.e., if $1 was "O" or "T").
echo "Chosen conversion ended with success. Obtaining file to copy in .../src/components/neural_model/model_data"
./exec_xxd.sh