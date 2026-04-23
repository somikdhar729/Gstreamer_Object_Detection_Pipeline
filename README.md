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