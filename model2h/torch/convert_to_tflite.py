import torch
import torch.nn as nn
import onnx
import tensorflow as tf
import os
import glob
import numpy as np
from PIL import Image
import shutil
import subprocess  # <--- Added to run onnx2tf
import onnxsim

####################################
# Model definitions (your code)
####################################

class ShallowEncoder(nn.Module):

    def __init__(self, in_ch, cout0, cout1, cout2):
        super(ShallowEncoder, self).__init__()

        self.conv0 = nn.Conv2d(in_channels=in_ch,  out_channels=cout0, kernel_size=3, padding=1, stride=1)
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

####################################
# Conversion Pipeline
####################################

CALIBRATION_DIR = "./ready_dataset"

def representative_dataset_gen():
    image_paths = glob.glob(os.path.join(CALIBRATION_DIR, "*.png"))
    if not image_paths:
        raise ValueError(f"No images found in '{CALIBRATION_DIR}'")
    
    print(f"--- Calibrating with {len(image_paths)} images ---")
    for path in image_paths:
        img = Image.open(path).convert("RGB")
        data = np.array(img).astype(np.float32) / 255.0
        # HWC -> CHW
        data = np.transpose(data, (2, 0, 1))
        # Add Batch -> BCHW
        data = np.expand_dims(data, axis=0)
        yield [data]

def convert_pipeline():
    print("[1/5] Loading PyTorch Model")
    model = uPydNet(in_ch=3, out_ch=1)
    if os.path.exists("upydnet_weights.pth"):
        model.load_state_dict(torch.load("upydnet_weights.pth", map_location="cpu"))
    model.eval()

    print("[2/5] Exporting to ONNX")
    dummy_input = torch.randn(1, 3, 48, 48)
    onnx_path = "model.onnx"
    onnx_simp_path = "model_simp.onnx"
    
    # Export raw
    torch.onnx.export(
        model, dummy_input, onnx_path,
        opset_version=17, # Using 17 for modern compatibility
        input_names=['input'], output_names=['output']
    )

    print("[3/5] Simplifying ONNX (Fixing WinError 2)")
    # We run simplification HERE in python to avoid subprocess errors
    onnx_model = onnx.load(onnx_path)
    model_simp, check = onnxsim.simplify(onnx_model)
    if not check:
        print("!! Warning: ONNX Simplifier check failed, but proceeding...")
    onnx.save(model_simp, onnx_simp_path)

    print("[4/5] Converting ONNX -> TensorFlow (via onnx2tf)")
    tf_path = "model_tf"
    
    # Command arguments explained:
    # -i: Input file (Simplified version)
    # -o: Output folder
    # -osd: Output SavedModel format
    # -ois: Overwrite Input Shape (FORCE static shape to fix axis errors)
    # -k: Keep parameters (helps with small models)
    cmd = [
        "onnx2tf",
        "-i", onnx_simp_path,
        "-o", tf_path,
        "-osd",
        "-ois", "input:1,3,48,48", 
        "-k", "input"
    ]
    
    subprocess.check_call(cmd)

    print("[5/5] Quantizing to TFLite (INT8)")
    converter = tf.lite.TFLiteConverter.from_saved_model(tf_path)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = representative_dataset_gen
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    
    tflite_model = converter.convert()

    tflite_filename = "../uPyD-Net.tflite"
    with open(tflite_filename, "wb") as f:
        f.write(tflite_model)

    print(f"SUCCESS! Model saved to {tflite_filename}")
    print(f"Size: {os.path.getsize(tflite_filename) / 1024:.2f} KB")

    # Cleanup
    if os.path.exists(tf_path): shutil.rmtree(tf_path)
    if os.path.exists(onnx_path): os.remove(onnx_path)
    if os.path.exists(onnx_simp_path): os.remove(onnx_simp_path)

if __name__ == "__main__":
    convert_pipeline()