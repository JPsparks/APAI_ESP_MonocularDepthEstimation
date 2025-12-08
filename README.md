# Architecture and Platform for Artificial Intelligence project
*by Pesaresi Jacopo*

** description **

## Generation of .h by traditional model definition file

1. First, move into `onnx2h`
```bash
cd model2h
```
2. Generate a virtual enviorement (venv, conda), then activate it

### ONNX path

!! this path should be updated, it is high likely that this path will fail

3. Install requirements 
```bash
pip install -r onnx/requirements.txt
```
4. Finally, run
```bash
./onnx/exec_onnx.sh
./exec_xxd.sh
```

### PyTorch path
3. Install requirements 
```bash
./torch/update_env.sh
```
4. Finally, run
```bash
./torch/exec_torch.sh
./exec_xxd.sh
```

#### Notice
After point 2, you could simply run the command
```
./converth.sh [M]
```

to concatenate .tflite definition and .h generation

**BUT before you have to setup correctly env**

(in other word, create the env, run the first command of the path, so run `./convert.sh`)


## Migrate on PlatformIO
*OLD: Output of previous step was directly redirect into the other dir, `PlatformIO`, ready to be copied with `main.cpp` into your env to push project into the ESP32!*

To make the code as customizable as possible, please manually copy and paste the previous result into the `src/components/neural_model/model_data` folder, taking care to do what is already possible to see in the folder itself (separation of the 2 definitions into .h and .cpp). This because if you need a different model, you should also create a new model class (see depth_estimation to understand better)

