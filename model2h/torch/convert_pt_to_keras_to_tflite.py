import os
# Force CPU usage to avoid CUDA 303 errors
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"

import torch
import torch.nn as nn
import tensorflow as tf
import numpy as np
import glob
from PIL import Image

# ================= CONFIGURATION IMPORT =================
WEIGHTS_FILE = os.getenv("TORCH_WEIGHTS", "upydnet_weights.pth")
OUTPUT_TFLITE = os.getenv("TFLITE_FILE", "uPyD_Net.tflite")
OUTPUT_TFLITE = "../" + OUTPUT_TFLITE
DEFAULT_CALIB_PATH = os.path.join("..", "calibration", "ready_dataset")
CALIBRATION_DIR = os.getenv("READY_DATASET_DIR", DEFAULT_CALIB_PATH)

IMG_HEIGHT = int(os.getenv("IMG_HEIGHT", 48))
IMG_WIDTH = int(os.getenv("IMG_WIDTH", 48))
INPUT_SHAPE = (IMG_HEIGHT, IMG_WIDTH, 3) # Keras uses NHWC

print(f"--- PYTHON CONFIG ---")
print(f"Weights:      {WEIGHTS_FILE}")
print(f"Output:       {OUTPUT_TFLITE}")
print(f"Dataset:      {CALIBRATION_DIR}")
print(f"Input Shape:  {INPUT_SHAPE}")
print(f"---------------------\n")
## second plot for a double check

# ================= PYTORCH LAYERS (BY Teacher Assistants work) =================

class ShallowEncoder(nn.Module):

    def __init__(self, in_ch, cout0, cout1, cout2):
        super(ShallowEncoder, self).__init__()

        self.conv0 = nn.Conv2d(in_channels=in_ch, out_channels=cout0, kernel_size=3, padding=1, stride=1)
        self.conv1 = nn.Conv2d(in_channels=cout0, out_channels=cout0, kernel_size=3, padding=1, stride=1)
        self.conv2 = nn.Conv2d(in_channels=cout0, out_channels=cout1, kernel_size=3, padding=1, stride=2)
        self.conv3 = nn.Conv2d(in_channels=cout1, out_channels=cout1, kernel_size=3, padding=1, stride=1)
        self.conv4 = nn.Conv2d(in_channels=cout1, out_channels=cout2, kernel_size=3, padding=1, stride=2)
        self.conv5 = nn.Conv2d(in_channels=cout2, out_channels=cout2, kernel_size=3, padding=1, stride=1)
        self.leakyrelu = nn.LeakyReLU(negative_slope=0.125)

    def forward(self, x):
        y0 = self.leakyrelu(self.conv0(x))
        y0 = self.leakyrelu(self.conv1(y0))
        pyd0 = y0
        y1 = self.leakyrelu(self.conv2(y0))
        y1 = self.leakyrelu(self.conv3(y1))
        pyd1 = y1
        y2 = self.leakyrelu(self.conv4(y1))
        y2 = self.leakyrelu(self.conv5(y2))
        pyd2 = y2
        return pyd0, pyd1, pyd2

class PDecoder(nn.Module):

    def __init__(self, in_ch, mid_ch, out_ch):
        super(PDecoder, self).__init__()

        self.conv0 = nn.Conv2d(in_channels=in_ch,  out_channels=mid_ch, kernel_size=3, padding=1, stride=1)
        self.conv1 = nn.Conv2d(in_channels=mid_ch, out_channels=mid_ch, kernel_size=3, padding=1, stride=1)
        self.conv2 = nn.Conv2d(in_channels=mid_ch, out_channels=out_ch, kernel_size=3, padding=1, stride=1)
        self.leakyrelu = nn.LeakyReLU(negative_slope=0.125)

    def forward(self, x):
        y = self.leakyrelu(self.conv0(x))
        y = self.leakyrelu(self.conv1(y))
        y = self.conv2(y)
        return y

class Upsampler(nn.Module):

    def __init__(self, in_ch, out_ch):
        super(Upsampler, self).__init__()
        self.tconv = nn.ConvTranspose2d(
            in_channels=in_ch,
            out_channels=out_ch,
            kernel_size=2,
            stride=2,
            padding=0,
            output_padding=0,
            groups=1
        )

    def forward(self, x):
        y = self.tconv(x)
        return y

class uPydNet(nn.Module):

    def __init__(self, in_ch, out_ch):
        super(uPydNet, self).__init__()

        self.encoder  = ShallowEncoder(in_ch, 8, 16, 32)
        self.decoder0 = PDecoder(32, 32, 32)
        self.decoder1 = PDecoder(48, 32, 32)
        self.decoder2 = PDecoder(40, 32, out_ch)
        self.ups0     = Upsampler(32, 32)
        self.ups1     = Upsampler(32, 32)
        self.relu     = nn.ReLU()

    def forward(self, x):
        pyd0, pyd1, pyd2 = self.encoder(x)
        dc0 = self.ups0(self.decoder0(pyd2))
        concat0 = torch.cat((pyd1, dc0), 1)
        dc1 = self.ups1(self.decoder1(concat0))
        concat1 = torch.cat((pyd0, dc1), 1)
        dc2 = self.decoder2(concat1)
        dc2 = self.relu(dc2)
        return dc2


# ================= KERAS MODEL DEFINITION (destination) =================
def create_keras_model():
    inputs = tf.keras.Input(shape=INPUT_SHAPE)
    
    # Helper layers
    leaky_relu = tf.keras.layers.LeakyReLU(negative_slope=0.125)
    
    def conv_block(x, filters, stride=1):
        x = tf.keras.layers.Conv2D(filters, 3, strides=stride, padding='same', use_bias=True)(x)
        return x

    # --- Encoder ---
    # conv0 (in->8)
    e_c0 = leaky_relu(conv_block(inputs, 8))
    # conv1 (8->8)
    e_c1 = leaky_relu(conv_block(e_c0, 8))
    pyd0 = e_c1
    
    # conv2 (8->16, s2)
    e_c2 = leaky_relu(conv_block(pyd0, 16, stride=2))
    # conv3 (16->16)
    e_c3 = leaky_relu(conv_block(e_c2, 16))
    pyd1 = e_c3
    
    # conv4 (16->32, s2)
    e_c4 = leaky_relu(conv_block(pyd1, 32, stride=2))
    # conv5 (32->32)
    e_c5 = leaky_relu(conv_block(e_c4, 32))
    pyd2 = e_c5
    
    # --- Decoder 0 ---
    # conv0 (32->32)
    d0_c0 = leaky_relu(conv_block(pyd2, 32))
    # conv1 (32->32)
    d0_c1 = leaky_relu(conv_block(d0_c0, 32))
    # conv2 (32->32)
    d0_out = conv_block(d0_c1, 32)
    
    # Upsample 0
    # PyTorch ConvTranspose2d k=2 s=2 -> Keras Conv2DTranspose k=2 s=2
    ups0_out = tf.keras.layers.Conv2DTranspose(32, 2, strides=2, padding='same')(d0_out)
    
    # Concat 0 (axis=3 per NHWC è l'equivalente di axis=1 per NCHW)
    concat0 = tf.keras.layers.Concatenate(axis=-1)([pyd1, ups0_out])
    
    # --- Decoder 1 ---
    d1_c0 = leaky_relu(conv_block(concat0, 32))
    d1_c1 = leaky_relu(conv_block(d1_c0, 32))
    d1_out = conv_block(d1_c1, 32)
    
    # Upsample 1
    ups1_out = tf.keras.layers.Conv2DTranspose(32, 2, strides=2, padding='same')(d1_out)
    
    # Concat 1
    concat1 = tf.keras.layers.Concatenate(axis=-1)([pyd0, ups1_out])
    
    # --- Decoder 2 (Output) ---
    d2_c0 = leaky_relu(conv_block(concat1, 32))
    d2_c1 = leaky_relu(conv_block(d2_c0, 32))
    d2_out = conv_block(d2_c1, 1) # Output channel = 1
    
    final_out = tf.keras.layers.ReLU()(d2_out)
    
    model = tf.keras.Model(inputs=inputs, outputs=final_out)

    return model


# ================= WEIGHTS UPLOAD =================
def transfer_weights(pt_model, keras_model):
    
    keras_layers = [l for l in keras_model.layers if isinstance(l, (tf.keras.layers.Conv2D, tf.keras.layers.Conv2DTranspose))]
    
    pt_modules = []
    
    # Encoder
    pt_modules.extend([pt_model.encoder.conv0, pt_model.encoder.conv1, 
                       pt_model.encoder.conv2, pt_model.encoder.conv3,
                       pt_model.encoder.conv4, pt_model.encoder.conv5])
    # Decoder0
    pt_modules.extend([pt_model.decoder0.conv0, pt_model.decoder0.conv1, pt_model.decoder0.conv2])
    # Ups0
    pt_modules.append(pt_model.ups0.tconv)
    # Decoder1
    pt_modules.extend([pt_model.decoder1.conv0, pt_model.decoder1.conv1, pt_model.decoder1.conv2])
    # Ups1
    pt_modules.append(pt_model.ups1.tconv)
    # Decoder2
    pt_modules.extend([pt_model.decoder2.conv0, pt_model.decoder2.conv1, pt_model.decoder2.conv2])
    
    if len(keras_layers) != len(pt_modules):
        raise ValueError(f"Mismatch layers! Keras: {len(keras_layers)}, PyTorch: {len(pt_modules)}")
        
    for i, (k_layer, pt_module) in enumerate(zip(keras_layers, pt_modules)):
        # Get PyTorch weights
        pt_w = pt_module.weight.detach().numpy() # Shape: (Out, In, H, W)
        pt_b = pt_module.bias.detach().numpy()   # Shape: (Out)
        
        if isinstance(pt_module, nn.ConvTranspose2d):
            # TRANSPOSE CONV:
            # PyTorch: (In, Out, H, W) -> Note: In/Out are inverted in PT ConvTranspose!
            # Keras:   (H, W, Out, In)
            # Permutation: (2, 3, 1, 0)
            k_w = pt_w.transpose(2, 3, 1, 0)
        else:
            # STANDARD CONV:
            # PyTorch: (Out, In, H, W)
            # Keras:   (H, W, In, Out)
            # Permutation: (2, 3, 1, 0)
            k_w = pt_w.transpose(2, 3, 1, 0)
            
        k_layer.set_weights([k_w, pt_b])
        print(f"Layer {i:>2}: Copied weights. PT shape ({','.join([f'{number:>3}' for number in pt_w.shape])}) -> Keras shape ({','.join([f'{number:>3}' for number in k_w.shape])})")


# ================= TFLITE CONVERSION =================
def representative_dataset_gen():
    search_path = os.path.join(CALIBRATION_DIR, "*.png")
    image_paths = glob.glob("../" + search_path)
    
    if not image_paths:
        print(f" !! WARNING: No images in {CALIBRATION_DIR}. Using random noise.")
        for _ in range(10):
            yield [np.random.rand(1, IMG_HEIGHT, IMG_WIDTH, 3).astype(np.float32)]
        return

    print(f" -> Calibrating with {len(image_paths)} images...")
    for path in image_paths:
        img = Image.open(path).convert("RGB")
        img = img.resize((IMG_WIDTH, IMG_HEIGHT))
        data = np.array(img).astype(np.float32) / 255.0
        yield [np.expand_dims(data, axis=0)] # NHWC


def main():
    # Load PyTorch
    pt_model = uPydNet(3, 1)
    if os.path.exists(WEIGHTS_FILE):
        print(f"Loading PT weights from {WEIGHTS_FILE}")
        pt_model.load_state_dict(torch.load(WEIGHTS_FILE, map_location="cpu"))
    else:
        print(f"!! ERROR: Weights file not found: {WEIGHTS_FILE}")
        return

    # Create Keras & Transfer
    keras_model = create_keras_model()
    transfer_weights(pt_model, keras_model)
    
    # Convert TFLite
    print("Converting to TFLite...")
    converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset_gen
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    
    tflite_model = converter.convert()
    
    # Save Output
    # We save to OUTPUT_TFLITE directly. 
    # If orchestrator expects it in root, we can write a relative path or the orchestrator will move it.
    # Usually better to save exactly where env var says.
    with open(OUTPUT_TFLITE, "wb") as f:
        f.write(tflite_model)
    
    print(f"SUCCESS! Saved to {OUTPUT_TFLITE}")

if __name__ == "__main__":
    main()