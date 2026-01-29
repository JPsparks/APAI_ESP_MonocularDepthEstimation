#!/bin/bash


### CONFIGURATION LOAD ###

if [ -f "./config.sh" ]; then
    source "./config.sh"
elif [ -f "../config.sh" ]; then
    source "../config.sh"
else
    echo "❌ Error: config.sh not found."
    exit 1
fi


### SETUP VARIABLES ###

# Input File
INPUT_FILE="${TFLITE_FILE}"

# Output Filenames derived from MODEL_NAME
H_FILENAME="${MODEL_NAME}.h"
CPP_FILENAME="${MODEL_NAME}.cpp"

H_PATH="${FINAL_OUTPUT_DIR}/${H_FILENAME}"
CPP_PATH="${FINAL_OUTPUT_DIR}/${CPP_FILENAME}"

# Macro Guard Generation
SANITIZED_NAME=$(echo "${MODEL_NAME}" | tr '[:lower:]' '[:upper:]' | tr -cd '[:alnum:]_')
MACRO_GUARD="__PW_APAI_MODEL_${SANITIZED_NAME}_H__"

# Standard C Variable Names
VAR_DATA="actModelPtr"
VAR_LEN="actModelLen"


### EXECUTION ###

echo ""
echo "### Executing XXD conversion (Split .h / .cpp) #########"
echo " -> Input:       $INPUT_FILE"
echo " -> Output H:    $H_PATH"
echo " -> Output CPP:  $CPP_PATH"

## Validation ##
if [ ! -f "$INPUT_FILE" ]; then
    echo "❌ Error: Input file '$INPUT_FILE' not found."
    exit 1
fi

## Prepare Output Directory ##
if [ ! -d "$FINAL_OUTPUT_DIR" ]; then
    mkdir -p "$FINAL_OUTPUT_DIR"
fi


### GENERATE .CPP SOURCE ###
echo " -> Generating source file (.cpp)..."

# Write includes and opening guard
{
    echo "#include \"${H_FILENAME}\""
    echo ""
    echo "#ifdef ${MACRO_GUARD}"
    echo ""
} > "$CPP_PATH"

# Run XXD WITHOUT -n flag (since your version doesn't support it)
# It will generate 'unsigned char uPyD_Net_tflite[] = ...'
xxd -i "$INPUT_FILE" >> "$CPP_PATH"

# POST-PROCESSING CPP

sed -i.bak \
    -e "s/^unsigned char .*\[\]/const unsigned char ${VAR_DATA}[]/g" \
    -e "s/^unsigned int .*_len/const unsigned int ${VAR_LEN}/g" \
    "$CPP_PATH" && rm "${CPP_PATH}.bak"

# Close guard
echo "" >> "$CPP_PATH"
echo "#endif" >> "$CPP_PATH"

### GENERATE .H HEADER ###
echo " -> Generating header file (.h)..."

{
    echo "#include \"MODEL_SELECTOR.h\""
    echo ""
    echo "#ifdef ${MACRO_GUARD}"
    echo "// Identifier: ${MODEL_NAME}"
    echo ""
    echo "extern const unsigned char ${VAR_DATA}[];"
    echo "extern const unsigned int ${VAR_LEN};"
    echo ""
    echo "#endif"
} > "$H_PATH"


echo "--------"
echo ""
echo "Success! Files generated in ${FINAL_OUTPUT_DIR}"
echo "   1. Add '#define ${MACRO_GUARD}' inside 'MODEL_SELECTOR.h' to enable this model."
echo "   2. Ensure 'actModelPtr' and 'actModelLen' are used in your main code."
echo ""