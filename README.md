# Architecture and Platform for Artificial Intelligence project
*by Pesaresi Jacopo*


** description **

## Generation of .h by onnx file

1. First, move into `onnx2h`
```bash
cd onnx2h
```
2. Generate a virtual enviorement (venv, conda), then activate it
3. Install requirements 
```bash
pip install -r requirements.txt
```
4. Finally, run
```bash
./convert.sh
```

## Migrate on PlatformIO
Output of previous step was directly redirect into the other dir, `PlatformIO`, ready to be copied with `main.cpp` into your env to push project into the ESP32!

