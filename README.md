# Gstreamer Object Detection Pipeline: 
Offline video object detection pipeline for benchmarking different inference backends.

Built using:
* GStreamer (decode/ encode)
* ONNX Runtime/ TensorRT (CPU & GPU inference)
* YOLO26 model

## Features
* End-to-end pipeline
```
Decode → Preprocess → Inference → Postprocess → Encode
```
* GStreamer-based I/O
* Modular backend(ONNX Runtime & TensorRT)
* Performance Benchmarking

## Current Status
| Component | Status |
| --------- | ------ |
| Decode (CPU - avdec_h264) | ✅ |
| Inference (ONNX Runtime CPU) | ✅ |
| Encode (CPU - x264) | ✅ |
## Development Environment
- **OS:** Windows 11
- **Compiler:** MSVC
- **Visual Studio:** Visual Studio 2022 Community
- **MSVC Toolset:** v143
- **Build System:** CMake 3.31.6

## Libraries / SDKs
- **GStreamer:** 1.28.1
- **TensorRT:** 10.15.1.29
- Ultralytics
- Python 3.13


## Model used: yolo26n

* To install ultralytics: [Installation steps](https://docs.ultralytics.com/quickstart/#conda-docker-image)

* Download yolo26n from the ultralytics website: [yolo26n](https://docs.ultralytics.com/models/yolo26/#supported-tasks-and-modes)
* Convert the yolo pytorch model to onnx format
    * Can be done using two ways:
        1. Using ultralytics onnx export: [yolo26 model onnx export](https://docs.ultralytics.com/integrations/onnx/#deploying-exported-yolo26-onnx-models)
        2. Run the following python code

            ```
            python scripts/pt_to_onnx_export.py <pytorch model source path> <onnx model destination path>
            ```

> ***In our case, I converted .pt model to .onnx model(simplified) using the second method.***

## To build the project
```
cmake .. -G "Visual Studio 17 2022"  
cmake --build . --config Release
cd Release
main.exe <input_video_path>  <model_path(.onnx/.engine)> <output_video_path>
```

## Runtime Setup(for Windows)
Copy the following into build/Release from the ONNX Runtime path:
* onnxruntime.dll
* onnxruntime_providers_shared.dll
* onnxruntime_providers_cuda.dll

## Benchmarking
* Video Info - Width: 1920, Height: 1080, FPS: 30

| Run | Decode Backend | Inference Backend | Draw Backend | Encode Backend | Model Format | Precision | Resolution | Decode (ms) | Inference (ms) | Draw (ms) | Encode (ms) | Total Time (s) | Frames | FPS |
|-----|----------------|-------------------|--------------|----------------|--------------|-----------|------------|-------------|----------------|-----------|-------------|----------------|--------|-----|
| 1   | CPU (FFmpeg avdec_h264) | CPU (ONNXRuntime) | CPU | CPU(x264) | ONNX   | FP32 | 1920x1080  | 0.7 | 93 | 0.02 | 1.3 | 183 | 1909 | 10.5 | 
| 2   | CPU (FFmpeg avdec_h264) | GPU (TensorRT) | CPU | CPU(x264) | Tensorrt   | FP32 | 1920x1080  | 0.6 | 8.5 | 0.02 | 1.4 | 20 | 1909 | 95 |
<!-- | 2   | GPU (NVDEC)    | GPU (TensorRT)    | CPU          | GPU (NVENC)    | TRT Engine   | FP16      | 1920x1080  |             |                |           |             |                |        |     |
| 3   | CPU (FFmpeg)   | GPU (ONNXRuntime) | CPU          | CPU            | ONNX         | FP32      | 1280x720   |             |                |           |             |                |        |     |
| 4   | GPU (NVDEC)    | CPU (ONNXRuntime) | CPU          | CPU            | ONNX         | FP32      | 640x480    |             |                |           |             |                |        |     | -->