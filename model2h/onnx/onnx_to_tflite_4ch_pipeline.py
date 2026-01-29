import os
import glob
import subprocess
import numpy as np
import onnx
from onnx import numpy_helper
import tensorflow as tf
from PIL import Image

# ================= CONFIGURATION IMPORT =================

MODEL_NAME_ENV = os.getenv("MODEL_NAME", "uPyD-Net")

# Input/Output Files
INPUT_ONNX = os.getenv("ONNX_FILE", f"{MODEL_NAME_ENV}.onnx")
OUTPUT_TFLITE = os.getenv("TFLITE_FILE", f"{MODEL_NAME_ENV}.tflite")
OUTPUT_TFLITE = "../" + OUTPUT_TFLITE
OUTPUT_HEADER = os.getenv("HEADER_FILE", f"{MODEL_NAME_ENV}.h")

# Intermediate Files (Derivati dal nome modello per pulizia)
PATCHED_ONNX = f"{MODEL_NAME_ENV}___4ch.onnx"
TF_MODEL_DIR = f"{MODEL_NAME_ENV}___tf_tmp"

# Paths
CALIBRATION_DIR = os.getenv("READY_DATASET_DIR", "../calibration/ready_dataset")

# Hardware Specifics
IMG_HEIGHT = int(os.getenv("IMG_HEIGHT", 48))
IMG_WIDTH = int(os.getenv("IMG_WIDTH", 48))


# Force CPU to avoid CUDA initialization errors
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"


print(f"--- CONFIGURATION DUMP ---")
print(f"Model Name:      {MODEL_NAME_ENV}")
print(f"Input ONNX:      {INPUT_ONNX}")
print(f"Output TFLite:   {OUTPUT_TFLITE}")
print(f"Calibration Dir: {CALIBRATION_DIR}")
print(f"Img Size:        {IMG_WIDTH}x{IMG_HEIGHT}")
print(f"--------------------------\n")




def step1_patch_onnx():
    print(f"\n[1/3] Patching ONNX: 3 Channels -> 4 Channels (Fixing Ambiguity)...")
    
    if not os.path.exists(INPUT_ONNX):
        raise FileNotFoundError(f"Source ONNX file not found: {INPUT_ONNX}")
    
    model = onnx.load(INPUT_ONNX)
    graph = model.graph

    # Modify Graph Input Shape
    input_tensor = graph.input[0]

    # Check current channels (Assuming NCHW: Batch, Channel, Height, Width)
    current_channels = input_tensor.type.tensor_type.shape.dim[1].dim_value
    
    if current_channels != 3:
        print(f" !! WARNING: Model has {current_channels} channels. Patch might be unnecessary.")
    
    # Set to 4 channels
    input_tensor.type.tensor_type.shape.dim[1].dim_value = 4
    input_name = input_tensor.name

    # Find the first Conv layer connected to input
    target_conv_node = None
    for node in graph.node:
        if input_name in node.input and node.op_type == "Conv":
            target_conv_node = node
            break
            
    if not target_conv_node:
        raise ValueError("Could not find the first Conv layer connected to input!")

    print(f" -> Patching layer: {target_conv_node.name}")

    # Patch Weights (Kernel)
    # Weights are typically the 2nd input
    weight_name = target_conv_node.input[1]
    
    # Find weight initializer
    weight_init = next(init for init in graph.initializer if init.name == weight_name)
    weights = numpy_helper.to_array(weight_init)
    
    # Pad axis 1 (Input Channels) with 0s. 
    # Shape: (Out, In, H, W). We want (Out, 3->4, H, W)
    # Pad config: ((0,0), (0,1), (0,0), (0,0)) -> Pad 1 zero at the end of axis 1
    new_weights = np.pad(weights, ((0,0), (0,1), (0,0), (0,0)), mode='constant', constant_values=0)
    
    print(f" -> Weights shape changed: {weights.shape} -> {new_weights.shape}")

    # Replace initializer
    new_weight_init = numpy_helper.from_array(new_weights, name=weight_name)
    graph.initializer.remove(weight_init)
    graph.initializer.append(new_weight_init)

    onnx.save(model, PATCHED_ONNX)
    print(f" -> Patched ONNX saved to: {PATCHED_ONNX}")

def step2_convert_to_tf():
    print(f"\n[2/3] Converting ONNX to TensorFlow (via onnx2tf)...")
    
    # We force input shape NCHW (1,4,48,48) because that's what our patched ONNX is.
    # onnx2tf will automatically transpose it to NHWC (1,48,48,4) for TensorFlow.
    cmd = [
        "onnx2tf",
        "-i", PATCHED_ONNX,
        "-o", TF_MODEL_DIR,
        "-osd",                                        # Output SavedModel
        "-ois", f"input:1,4,{IMG_HEIGHT},{IMG_WIDTH}"  # Force static input shape
    ]
    
    try:
        subprocess.check_call(cmd)
        print(" -> TensorFlow SavedModel generated successfully.")
    except subprocess.CalledProcessError as e:
        print(f" !! Error in onnx2tf: {e}")
        exit(1)

def step3_quantize_tflite():
    print(f"\n[3/3] Converting to TFLite (INT8 Quantization)...")

    # --- Representative Dataset Generator ---
    # Crucial: Reads 3-channel images and adds the 4th dummy channel on the fly.
    def representative_dataset_gen():
        image_paths = glob.glob(os.path.join(CALIBRATION_DIR, "*.png"))
        
        if not image_paths:
            print(f" !! WARNING: No images found in {CALIBRATION_DIR}. Using random data.")
            for _ in range(10):
                # Random data 48x48x4
                yield [np.random.rand(1, IMG_HEIGHT, IMG_WIDTH, 4).astype(np.float32)]
            return

        print(f" -> Calibrating with {len(image_paths)} images...")
        for path in image_paths:
            img = Image.open(path).convert("RGB")
            # Resize just in case
            img = img.resize((IMG_WIDTH, IMG_HEIGHT))
            
            # Convert to numpy and normalize
            data_rgb = (np.array(img).astype(np.float32) / 127.5) - 1.0 #data_rgb = np.array(img).astype(np.float32) / 255.0 # Shape: (48, 48, 3)
            
            # === THE TRICK: Add 4th Channel (Padding) ===
            # Pad the last axis (Channels) with 1 zero.
            # ((0,0), (0,0), (0,1)) -> Pad axis 2
            data_rgba = np.pad(data_rgb, ((0,0), (0,0), (0,1)), mode='constant', constant_values=0)
            
            # Add batch dimension -> (1, 48, 48, 4)
            data_batch = np.expand_dims(data_rgba, axis=0)
            
            yield [data_batch]

    try:
        converter = tf.lite.TFLiteConverter.from_saved_model(TF_MODEL_DIR)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        
        converter.representative_dataset = representative_dataset_gen
        converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
        converter.inference_input_type = tf.int8
        converter.inference_output_type = tf.int8
        
        tflite_model = converter.convert()
        
        with open(OUTPUT_TFLITE, "wb") as f:
            f.write(tflite_model)
            
        size_kb = os.path.getsize(OUTPUT_TFLITE) / 1024
        print(f" -> SUCCESS! TFLite Saved: {OUTPUT_TFLITE} ({size_kb:.2f} KB)")
        
    except Exception as e:
        print(f" !! Error during TFLite conversion: {e}")
        exit(1)

def cleanup():
    print(f"\nBONUS: Cleanup...")
    # Optional: Remove intermediate files
    if os.path.exists(PATCHED_ONNX):
        os.remove(PATCHED_ONNX)
        print(f" -> Removed {PATCHED_ONNX}")
    # We usually keep the TF model for debugging, but you can uncomment below
    # import shutil
    # if os.path.exists(TF_MODEL_DIR): shutil.rmtree(TF_MODEL_DIR)

if __name__ == "__main__":
    step1_patch_onnx()
    step2_convert_to_tf()
    step3_quantize_tflite()
    # cleanup()
    print("\n--- PIPELINE COMPLETE ---")