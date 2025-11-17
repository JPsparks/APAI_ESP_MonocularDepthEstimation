
### libraries

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import numpy as np
import onnx
from onnx import numpy_helper

### manual definition of the model

print("\n[1/4] Definition of the model...")

def create_upyd_net(input_shape=(48,48,3)):
    """
    Input: 48x48x3
    """
    inputs = keras.Input(shape=input_shape, name='input')
    # === ENCODER ===
    # Conv0 (3 -> 8) 48x48
    x = layers.Conv2D(
        8, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_SencoderSconv0SConv_output_0')(inputs)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_SencoderSleakyreluSLeakyRelu_output_0')(x)
    # Conv1 (8 -> 8) 48x48
    x = layers.Conv2D(
        8, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_SencoderSconv1SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_SencoderSleakyrelu_1SLeakyRelu_output_0')(x)
    skip_conv1 = x   
    
    # Conv2 (8 -> 16) stride=2 -> 24x24
    x = layers.Conv2D(
        16, (3,3), strides=(2,2), padding='same', use_bias=True,
        name='Conv_SencoderSconv2SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_SencoderSleakyrelu_2SLeakyRelu_output_0')(x)

    # Conv3 (16 -> 16) 24x24  <-- skip for concat1
    x = layers.Conv2D(
        16, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_SencoderSconv3SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_SencoderSleakyrelu_3SLeakyRelu_output_0')(x)
    skip_conv3 = x   # shape: (24,24,16)

    # Conv4 (16 -> 32) stride=2 -> 12x12
    x = layers.Conv2D(
        32, (3,3), strides=(2,2), padding='same', use_bias=True,
        name='Conv_SencoderSconv4SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_SencoderSleakyrelu_4SLeakyRelu_output_0')(x)

    # Conv5 (32 -> 32) 12x12
    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_SencoderSconv5SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_SencoderSleakyrelu_5SLeakyRelu_output_0')(x)

    # === DECODER 0 (work with 12x12) ===
    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder0Sconv0SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_Sdecoder0SleakyreluSLeakyRelu_output_0')(x)

    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder0Sconv1SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_Sdecoder0Sleakyrelu_1SLeakyRelu_output_0')(x)

    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder0Sconv2SConv_output_0')(x)

    # Upsample: ConvTranspose 2x2 stride=2 -> 12x12 -> 24x24
    x = layers.Conv2DTranspose(
        32, (2,2), strides=(2,2), padding='valid', use_bias=True,
        name='ConvTranspose_Sups0StconvSConvTranspose_output_0')(x)  # -> (24,24,32)

    # === DECODER 1 ===
    x = layers.Concatenate(name='concat1')([x, skip_conv3])  # 32 + 16 = 48 channels

    # Conv decoder1_conv0: in_channels=48 -> out=32
    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder1Sconv0SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_Sdecoder1SleakyreluSLeakyRelu_output_0')(x)

    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder1Sconv1SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_Sdecoder1Sleakyrelu_1SLeakyRelu_output_0')(x)

    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder1Sconv2SConv_output_0')(x)

    # Upsample: ConvTranspose 2x2 stride=2 -> 24x24 -> 48x48
    x = layers.Conv2DTranspose(
        32, (2,2), strides=(2,2), padding='valid', use_bias=True,
        name='ConvTranspose_Sups1StconvSConvTranspose_output_0')(x)  # -> (48,48,32)

    # === DECODER 2 ===
    # concat with skip from encoder conv1 (48x48,8ch)
    x = layers.Concatenate(name='concat2')([x, skip_conv1])  # 32 + 8 = 40 channels

    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder2Sconv0SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_Sdecoder2SleakyreluSLeakyRelu_output_0')(x)

    x = layers.Conv2D(
        32, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder2Sconv1SConv_output_0')(x)
    x = layers.LeakyReLU(alpha=0.125,
                         name='LeakyRelu_Sdecoder2Sleakyrelu_1SLeakyRelu_output_0')(x)

    # Final conv -> 1 channel
    x = layers.Conv2D(
        1, (3,3), strides=(1,1), padding='same', use_bias=True,
        name='Conv_Sdecoder2Sconv2SConv_output_0')(x)

    # Final ReLU
    outputs = layers.ReLU(name='Relu_66')(x)

    model = keras.Model(inputs=inputs, outputs=outputs, name='uPyD_Net_recreated')
    return model

model = create_upyd_net((48,48,3))
# model.summary(line_length=120)




#### import parameters

print("\n[2/4] Upload weights by ONNX...")


def onnx_to_tf_layer(onnx_name):
    if "tconv" in onnx_name:
        # TransposeConv
        parts = onnx_name.split(".")  # ups0, tconv, weight
        block, conv = parts[0], parts[1]
        tf_layer = f"ConvTranspose_S{block}S{conv}SConvTranspose_output_0"
    else:
        parts = onnx_name.split(".")  # encoder, conv0, weight
        block, conv = parts[0], parts[1]
        tf_layer = f"Conv_S{block}S{conv}SConv_output_0"

    # Parametro
    if onnx_name.endswith("weight"):
        param = "kernel"
    else:
        param = "bias"

    return tf_layer, param



try:
    onnx_model = onnx.load("uPyD-Net.onnx")

    # Estrai inizializzatori
    weights_dict = {init.name: numpy_helper.to_array(init)
                    for init in onnx_model.graph.initializer}

    print(f"Get {len(weights_dict)} tensors by ONNX")

    loaded_count = 0

    for onnx_name, weight in weights_dict.items():

        if "weight" in onnx_name and weight.ndim == 4:
            weight = np.transpose(weight, (2,3,1,0))

        tf_layer_name, param_name = onnx_to_tf_layer(onnx_name)

        try:
            layer = model.get_layer(tf_layer_name)
        except:
            print(f"⚠ TF layer not found: {tf_layer_name} (by {onnx_name})")
            continue

        current = layer.get_weights()

        if len(current) != 2:
            print(f"⚠ Layer {tf_layer_name} does not have predicted weights (has {len(current)})")
            continue

        # Sostituzione
        if param_name == "kernel":
            new_w = [weight, current[1]]
        else:
            new_w = [current[0], weight]

        layer.set_weights(new_w)
        loaded_count += 1

    print(f"Update correctly done: {loaded_count}/{len(weights_dict)} done")

except Exception as e:
    print(f"General error: {e}")




######### inferece test

print("\n[3/4] Test inferenza...")

try:
    # Input di test
    test_input = np.random.randn(1, 48, 48, 3).astype(np.float32)
    
    # Inferenza
    output = model.predict(test_input, verbose=0)
    
    print(f"Inference complete")
    print(f"\tInput shape: {test_input.shape}")
    print(f"\tOutput shape: {output.shape}")
    print(f"\tOutput range: [{output.min():.4f}, {output.max():.4f}]")
    
except Exception as e:
    print(f"Error in inference time: {e}")






print("\n[4/4] Generate tflite file ...")

try:
    # # 3. TFLite FLOAT32
    # converter = tf.lite.TFLiteConverter.from_keras_model(model)
    # converter.optimizations = [tf.lite.Optimize.DEFAULT]
    # tflite_model = converter.convert()

    # with open('model.tflite', 'wb') as f:
    #     f.write(tflite_model)
    
    # tflite_size = len(tflite_model) / 1024 / 1024
    
    # 4. TFLite quantizzato INT8
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]

    # Dataset rappresentativo
    def representative_dataset():
        for _ in range(100):
            yield [np.random.randn(1, 48, 48, 3).astype(np.float32)]
    
    converter.representative_dataset = representative_dataset
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.uint8
    converter.inference_output_type = tf.uint8
    
    tflite_quant = converter.convert()
    
    with open('uPyD-Net.tflite', 'wb') as f:
        f.write(tflite_quant)
    
    quant_size = len(tflite_quant) / 1024 / 1024
    print(f"TFLite INT8 saved: uPyD-Net.tflite ({quant_size:.2f} MB)")
    
except Exception as e:
    print(f"⚠ Error during generation: {e}")

print("\n\nConversion complete! Now exec xxd to gain correct file")