#!/bin/bash

# # Virtual environment name
# ENV_NAME="venv_convert"
# PYTHON_CMD="python" # Python command to use, can be changed to 'python' if necessary

# # 0. Check for and Create the Virtual Environment if it doesn't exist
# echo "---"
# if [ ! -d "$ENV_NAME" ]; then
#     echo "Virtual environment $ENV_NAME not found."
#     echo "Creating it with $PYTHON_CMD -m venv $ENV_NAME..."
#     # If the python3 command is not available, try 'python'
#     if command -v $PYTHON_CMD &> /dev/null; then
#         $PYTHON_CMD -m venv $ENV_NAME
#         # Check if creation failed
#         if [ $? -ne 0 ]; then
#             echo "ERROR: Virtual environment creation failed. Check that $PYTHON_CMD is installed."
#             exit 1
#         fi
#         echo "Environment $ENV_NAME created successfully."
#     else
#         echo "ERROR: $PYTHON_CMD command not found. Ensure Python is installed and in your PATH."
#         exit 1
#     fi
# else
#     echo "Virtual environment $ENV_NAME already exists."
# fi

# # 1. Find and Activate the virtual environment
# echo "---"
# echo "Activating virtual environment $ENV_NAME..."

# # Attempt activation for Linux/macOS (bin)
# if [ -f "$ENV_NAME/bin/activate" ]; then
#     source "$ENV_NAME/bin/activate"
#     echo "Environment activated (Linux/macOS)."
# # Attempt activation for Windows (Scripts)
# elif [ -f "$ENV_NAME/Scripts/activate" ]; then
#     source "$ENV_NAME/Scripts/activate"
#     echo "Environment activated (Windows)."
# else
#     echo "ERROR: Cannot find the 'activate' file in $ENV_NAME/bin or $ENV_NAME/Scripts."
#     exit 1 # Exit script with an error code
# fi

# # 2. Install dependencies from requirements.txt
# echo "---"
# echo "Checking for and installing dependencies from requirements.txt..."

# if [ -f "requirements.txt" ]; then
#     # '-q' for cleaner output, useful in an automated script
#     pip install -r requirements.txt -q
#     if [ $? -ne 0 ]; then
#         echo "ERROR: Dependency installation failed. Check the requirements.txt file."
#         exit 1
#     fi
#     echo "Dependencies installed or already satisfied."
# else
#     echo "WARNING: requirements.txt file not found. No dependencies installed."
# fi

# 3. Run your Python script
echo "---"
echo "Executing the convert.py script..."
# Use 'python' as the virtual environment redirects it correctly
python convert.py

# # 4. Deactivate the virtual environment (optional, but good practice)
# echo "---"
# if command -v deactivate &> /dev/null; then
#     deactivate
#     echo "Deactivating the virtual environment."
# fi

echo "---"
echo "Completed."