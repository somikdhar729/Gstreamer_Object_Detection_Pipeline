# Gstreamer_Object_Detection_Pipeline

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


### Model used: yolo26n

> To install ultralytics: [Installation steps](https://docs.ultralytics.com/quickstart/#conda-docker-image)

* Download yolo26n from the ultralytics website: [yolo26n](https://docs.ultralytics.com/models/yolo26/#supported-tasks-and-modes)
* First convert the yolo pytorch model to onnx

```
python scripts/pt_to_onnx_export.py <pytorch model source path> <onnx model destination path>
```

## To build the project
```
cmake .. -G "Visual Studio 17 2022"  
cmake --build . --config Release
cd Release
main.exe <input_video_path>  <model_path(.onnx/.engine)> <output_video_path>
```

Note: For proper configuration, copy   ***onnxruntime.dll, onnxruntime_providers_shared.dll, onnxruntime_providers_cuda.dll*** to build/Release

## Benchmarking
* Video Info - Width: 1920, Height: 1080, FPS: 30

| Run | Decode Backend | Inference Backend | Draw Backend | Encode Backend | Model Format | Precision | Resolution | Decode (ms) | Inference (ms) | Draw (ms) | Encode (ms) | Total Time (s) | Frames | FPS |
|-----|----------------|-------------------|--------------|----------------|--------------|-----------|------------|-------------|----------------|-----------|-------------|----------------|--------|-----|
| 1   | CPU (FFmpeg avdec_h264) | CPU (ONNXRuntime) | CPU | CPU(x264) | ONNX   | FP32 | 1920x1080  | 0.7 | 93 | 0.02 | 1.3 | 183 | 1909 | 10.5 |
<!-- | 2   | GPU (NVDEC)    | GPU (TensorRT)    | CPU          | GPU (NVENC)    | TRT Engine   | FP16      | 1920x1080  |             |                |           |             |                |        |     |
| 3   | CPU (FFmpeg)   | GPU (ONNXRuntime) | CPU          | CPU            | ONNX         | FP32      | 1280x720   |             |                |           |             |                |        |     |
| 4   | GPU (NVDEC)    | CPU (ONNXRuntime) | CPU          | CPU            | ONNX         | FP32      | 640x480    |             |                |           |             |                |        |     | -->