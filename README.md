# Porting uPyDnet to ESP32-S3 with MicroTFLite

> **Abstract**
> This project documents the miniaturization and deployment process of the **uPyDnet** neural network, originally developed in an academic setting and adapted here in collaboration with PhD students from the *Architectures and Platforms for Artificial Intelligence* course, on the **[Freenove ESP32-S3 CAM](https://freenove.com/fnk0085)** board. The goal is to demonstrate the feasibility of executing complex tasks like *Monocular Depth Estimation* on commercial embedded hardware, leveraging modern conversion and optimization frameworks for Edge AI.

*Process demo (from flashing to execution):*

![demo](./images/gifVideo.gif)

(see `./images/video.mp4` to watch better this animation)

> NOTICE: go near to the end of the repo to replicate on your device the above animation!


## 1. Introduction

The main objective of this project is to practically apply the concepts of neural network miniaturization learned during the university course **[Architectures and Platforms for Artificial Intelligence](https://www.unibo.it/en/study/course-units-transferable-skills-moocs/course-unit-catalogue/course-unit/2025/446607), module 2**.
The model was proposed by the course's PhD team and consists of a *Monocular Depth Estimation* network.

### Context: The uPyDnet Network

*Monocular Depth Estimation* networks estimate a "depth map" from a single RGB image. Compared to stereo vision or LiDAR sensors, using a single neural network drastically reduces hardware costs and complexity.

The chosen network, **uPyDnet**, is a natively *ultra-lightweight* architecture designed to accept **48x48 pixel** RGB images as input. It was created for integration into high-performance but resource-constrained *embedded* devices (such as PULP processors). The version used in this project was trained on the **KITTI** dataset (*Autonomous Driving* scenario).

The challenge was adapting this model through quantization and conversion to run on *commodity* hardware like the ESP32-S3, analyzing the impact of individual operations on CPU clock cycles.

### Repository Structure

The project is divided into three logical phases:

1. **`model2h`**: Pipeline for model miniaturization and conversion.
2. **`PlatformIO`**: Firmware for injecting the model and executing it on the ESP32-S3.
3. **`quality_check`**: Tools for verification, result visualization, and profiling.

---

## 2. Hardware Configuration and Camera Management

The firmware, contained in the `PlatformIO` folder, was developed specifically for the **ESP32-S3 CAM** board. However, the code structure (particularly the configuration header files) should be generic enough to be adapted to other camera-equipped boards by modifying the macros in the `.h` files.

### Camera Management and Preprocessing

To ensure optimal input for the neural network, the camera is configured to acquire images at **240x240** resolution. Although the sensor supports higher resolutions, this choice allows for clean integer downsampling.
The image is reduced to **48x48** (the input required by uPyDnet) using a **5x5 sliding window** algorithm: the mask slides over the original image without overlapping, sampling exclusively the central pixel. This approach reduces computational cost compared to bilinear interpolation while retaining sufficient spatial information for the network.

---

## 3. Model Miniaturization (`model2h`)

Half of the work in this project involved transforming the network weights into C++ files compatible with the **MicroTFLite** (TensorFlow Lite for Microcontrollers) framework.

### 4.1 Python Environment Setup

The `model2h` folder contains the `update_env.sh` script. This sets up the environment by installing dependencies for both ONNX and Torch pipelines (it assumes a `venv` or `conda` virtual environment is already active).

### 4.2 Dataset Generation (Calibration)

For efficient conversion (especially for `int8` quantization), a "Representative Dataset" is required. Images were extracted from the [Indoor Scenes CVPR 2019](https://www.kaggle.com/datasets/itsahmad/indoor-scenes-cvpr-2019?resource=download) dataset.
*Note:* Any dataset of real scenes is valid for statistical calibration of activation ranges, regardless of the network being trained on KITTI. The images are processed with the same 240x240  48x48 cropping used on the ESP32.

### 4.3 Conversion Mechanism

The system supports two pipelines:

#### A. ONNX Pipeline (Generally Recommended)

Load the `.onnx` model into `./model2h/onnx`. This phase leverages **`onnx2tf`**, the *de facto* standard for converting complex models, mainly because it does not require redefining the model and is generally easily reusable.

> **Technical note on channels:** 3x3 convolutional kernels on 3 channels (RGB) can cause memory misalignment or ambiguity in dimension interpretation (H, W, C) by converters. To overcome this, a **dummy fourth channel** was inserted before conversion. In the ESP32 firmware, this fourth channel is artificially injected into the tensor input.

> **$\color{red}{\text{WARNING}}$**: Despite multiple efforts and attempts, this path was not found to be ideal for the chosen model. The implementation adding a dummy fourth dimension has been left as it could serve as inspiration, but **for executing this project, it is suggested to follow the other path.** This will be surelly one of the main problem problem to solve as soon as possible

#### B. PyTorch/Weights Pipeline

Requires manual redefinition of the architecture in the `convert_pt_to_keras_to_tflite.py` script, but it is impossible to fail with this methodology. By modifying `create_keras_model()`, the topology is forced to ensure a 1:1 conversion of the `.pth` weights. In short, although more verbose, this path offers total control over the graph structure.

### 4.4 Header Generation

The `exec_xxd.sh` script uses `xxd` to convert the `.tflite` into a Hex Dump, separating declarations (`.h`) from definitions (`.cpp`) to optimize compilation times.

---

## 5. Injection and Execution (`PlatformIO`)

This section describes the firmware compilable via **PlatformIO**.

### Folder Structure

* **`test_code`**: Code to validate PSRAM, Camera, and SD Card. *Credits: derived from [Freenove](https://docs.freenove.com/projects/fnk0085/en/latest/) tutorials*.
* **`OOP_NO_TFLite`**: "Skeleton" version (without neural network inference) for debugging application logic without any overhead due to the compilation of the MicroTFLite library.
* **`OOP_TFLite`**: **Main Project**.

### Setup Instructions (`./OOP_TFLite`)

The definitive source code is in `./PlatformIO/OOP_TFLite`.

0. **Creation:** Create a PlatformIO project (select your board, but this shouldn't be a blocking choice as board specs will be overwritten by the next step), and, when it is done, essentially substitute src dir generated into the PlatformIO project with the content of `./PlatformIO/OOP_TFLite/src`.
1. **Configuration:** Replace the default generated `platformio.ini` with the one present in this folder.
2. **Weight Import:** Copy the generated `.h` and `.cpp` files (from `./model2h/result_to_move`) into `./src/components/neural_model/model_data/`.
3. **Model Selection:** Go to `config.h` in the `./src` root to activate the macro for the model generated in the previous paragraph (note: it is suggested to change the automatically generated tag if not already done).
4. **Pipeline Configuration:** In `./src/config.h`, besides the correct model, you must enable the correct macro:
* `#define USING_ONNX` (activates 4-channel padding).
* `#define USING_TORCH` (standard 3-channel input).



> **Note:** The project already includes a pre-loaded and functioning model. The steps above are necessary only to update the neural network with a custom one. But otherwise it is enought only point 0 and 1

---

## 6. Result Verification (`quality_check`)

To validate inference, the firmware saves four files to the SD Card for each shot:

1. **RAW JPEG**: Original 240x240 image.
2. **RGB888**: Uncompressed conversion (ground truth).
3. **Input Network**: Downscaled 48x48 image.
4. **Depth Map**: Network output (grayscale 0-255).

(Refer back to the beginning gif to better understand what is meant)

These photos can be copy-pasted from the SD to the computer to be studied, specifically in the `quality_check/img` folder of this repo, and "analyzed" by the paired `quality_check.ipynb` file.

### Qualitative Analysis

Since it is difficult to judge what the model actually considered far or near, the Python Notebook in `quality_check` overlays the *Depth Map* on the original image, generating a **heat-map** (with red indicating areas estimated as "far").

Below a quick example:
![Esempio](images/Example_res.jpg)

### Cycle Profiling

Leveraging **MicroTFLite**'s built-in profiler integration, it was possible to analyze the computational load distribution across the various network layers:

![Chart](images/inference_profiling.png)

**Chart Analysis:**
The pie chart (right) and logarithmic histogram (left) clearly highlight the architecture's bottlenecks on ESP32 hardware:

1. **Convolution Dominance:** As expected, most cycles are spent in `CONV_2D` operations (see color $\color{red}{\text{red}}$).
2. **Upsampling Impact:** A very interesting data point is the huge impact of `TRANSPOSE_CONV` (see color $\color{orange}{\text{orange}}$). Although numerically few compared to other layers, they occupy a third of the total inference time. These operations are crucial for the image "decoding" phase to restore it to the original (or near-original) depth map size.
3. **Activation Efficiency:** The `LEAKY_RELU` and `PACK` operations (in $\color{azure}{\text{azure}}$ and $\color{yellow}{\text{yellow}}$) have a negligible impact (< 3%), demonstrating that the overhead introduced by non-linear activation functions is minimal on this architecture.

This analysis suggests that future optimizations should focus on efficient implementation or replacement (e.g., via *resize-convolution*) of Transpose Convolution layers.

### Other Performance Reflections

In addition to procedure counters and a subjective quality analysis, we can report these final metrics and specifications with the code defined and tested so far on **Freenove ESP32-S3**:

* **Total Cycle Time:** A complete iteration (photo capture, preprocessing, inference, saving to SD) takes approximately **6-7 seconds** (generally around 6200ms).
This was achieved by keeping the `TIME_COUNT` flag active in the `config.h` file present in the firmware.
* **Interaction:** Acquisition is triggered by pressing the **BOOT** button integrated on the board, maximizing project portability.


## 7. Recap for Reproducibility

Now that you understand the general structure of the repository, this section provides a step-by-step guide on how to reproduce the results or adapt the pipeline for **custom neural networks**.

### Step 1: From "Your Model" to C++ Headers (`.h`)

This phase transforms a trained model into a format the ESP32 can compile.

1. **Prerequisites: Calibration Dataset**
Having sample data that represents what the network will actually see is crucial for the quantization process.
    * *For Image Tasks:* The repository already includes a robust pipeline able to generate 48x48 3-channel images, in a way that mirror how we generate those on the ESP (this is crucial to follow what i've wrote before).
    * *For Custom Tasks:* If your network accepts different inputs (e.g., non-image data, or image with different resolutions), you must modify `./model2h/calibration/get_suitable_dataset.py`, specifically the `process_image(...)` function (and potentially its signature). Next you have to ensure the new dataset is correctly imported by the subsequent conversion scripts.

2. **Conversion: From Model to `.tflite`**:
choose the kind of conversion you want follow (with advantages/disadvantage you seen before): **ONNX** or **PyTorch weights**.
    * **Option A: ONNX Path**
      1. Place your `.onnx` file in `./model2h/onnx`.
      2. Adapt the script `./model2h/onnx/onnx_to_tflite_4ch_pipeline.py` to your network's specific needs (e.g., do you strictly need the 4th dummy channel generation? Does it require different input shapes?).

    * **Option B: PyTorch Path**
      1. Place your `.pth` weights in `./model2h/torch`.
      2. **Mandatory:** Adapt `./model2h/torch/convert_pt_to_keras_to_tflite.py` by modifying the `create_keras_model(...)` function to essentially replicate your network architecture using Keras layers. Despite the manual verbosity, this method guarantees 100% conversion reliability.

3. **Header Generation: From `.tflite` to `.h`** (and .cpp):
If the previous steps succeed, a `.tflite` file will be generated in the root of `./model2h`. The final step is running `xxd` to convert this binary into C-arrays ready for PlatformIO.

#### Executing the Pipeline

To automate these steps, follow this workflow:

1. **Environment Setup:** Create a `venv` or `conda` environment. Activate it and install dependencies by running:

```bash
./model2h/update_env.sh
```

2. **Configuration:** If you added new files or changed filenames, update the variables in `./model2h/config.sh` to point to your new assets.

3. **Execution:** Use the orchestrator script to run the entire pipeline described before automatically.
Just follow this command

```bash
# View help and options 
./convert.sh -h

# Run the ONNX pipeline
./convert.sh O
# OR run the PyTorch pipeline
./convert.sh T
############################################
# NOTICE: you should (and it's enought to) #
# run ONLY ONE between above command,      #
# unless you don't choose to use both      #
# conversion style                         #
############################################
```

4. **Result Retrieval:** Upon success, navigate to `./model2h/result_to_move`. You will find the generated `.h` and `.cpp` files there.
> **Notice:** Check the macro definitions inside these files. Ensure the automatically generated variable names suit your project, or rename them if necessary.



### Step 2: Let the ESP Do the Job

If you are just reproducing the project, refer back to the *Setup Instructions* in Chapter 5. However, if you are deploying a **custom network for a new task**, consider the following advice:

* **Code Reuse:** Leverage the existing class structure and macros defined in `.../src/components/neural_model`.
* **Configuration:** Reuse the `platformio.ini` configuration. Significant effort went into ensuring the correct library imports and linker flags, so use it as a solid baseline.
* **Debugging:** Use the test mains located in `./PlatformIO/test_code` to isolate issues with hardware components (Camera, SD, PSRAM) before running the full model.

**Adapting to New Inputs:**
If your network requires non-image inputs or specific normalization:

1. Look at the inheritance/overriding technique used between `.../src/components/neural_model/local_model.h` (the base interface) and `depth_estimation_t.h` & `.cpp` / `depth_estimation_o.h` & `.cpp` (the concrete implementation).
2. Create your own implementation (e.g., `my_custom_model.h`) that overrides the `inference()` and `decode_inference()` or data loading methods.

> **Notice:** This project implements both ONNX and TORCH paths to demonstrate flexibility. If you bring your own network, you don't need to maintain this duality. You can simply (re)define `local_model.h` once. If you do keep multiple implementations, ensure they are correctly conditionally imported in `main.cpp` (see lines 24-34 to take inspiration).

> **Crucial Warning:** I spent days debugging an issue where the ESP32 camera (at least this specific model) could not natively capture images in the exact format the network expected. Always double-check that your preprocessing code (on the ESP32) correctly adapts the raw sensor data to the input tensor requirements of your custom network, otherwise you have to preprocess differentely your data before passing them to the network.
---
## References and Publications

This work uses various tutorials found online, primarily the one downloadable from Freenove, but is rooted in the following research works:

* **Original Network (uPyDnet/PyDnet):**
F. Aleotti, F. Tosi, M. Poggi, S. Mattoccia, *"PyDnet: Real-time Monocular Depth Estimation on Embedded Platforms"*, in IEEE Transactions on Intelligent Transportation Systems, 2021.
[IEEE Xplore](https://ieeexplore.ieee.org/abstract/document/9422776)
* **Hardware-Aware Optimization Techniques:**
M. Risso, A. Burrello, L. Benini, et al., *"Pruning In Time (PIT): A Lightweight Network Architecture Optimizer for Temporal Convolutional Networks"*, in Design, Automation & Test in Europe (DATE), 2020.
[Google Scholar](https://scholar.google.com/citations?view_op=view_citation&hl=it&user=QotA2soAAAAJ&citation_for_view=QotA2soAAAAJ:zYLM7Y9cAGgC)