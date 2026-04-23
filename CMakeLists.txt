cmake_minimum_required(VERSION 3.25)
project(Gstreamer_Object_Detection_Pipeline)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

################################## Engine Conversion ##################################
# Set TensorRT path
set(TENSORRT_ROOT "C:/TensorRT-10.15.1.29")
# Set ONNX Runtime path
set(ONNXRUNTIME_ROOT "C:/onnxruntime-win-x64-gpu-1.24.1")
# set GStreamer path
set(GSTREAMER_ROOT_DIR "C:/Program Files/gstreamer/1.0/msvc_x86_64")

# CUDA
cmake_policy(SET CMP0146 OLD)
# find_package(CUDA REQUIRED)
enable_language(CUDA)
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
    ${CUDA_INCLUDE_DIRS}
    ${TENSORRT_ROOT}/include 
    ${ONNXRUNTIME_ROOT}/include 
    ${GSTREAMER_ROOT_DIR}/include
    ${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0
    ${GSTREAMER_ROOT_DIR}/include/glib-2.0
    ${GSTREAMER_ROOT_DIR}/lib/glib-2.0/include
    
)

link_directories(
    ${TENSORRT_ROOT}/lib
    ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64
    ${ONNXRUNTIME_ROOT}/lib
     ${GSTREAMER_ROOT_DIR}/lib
)

# Add main executable
add_executable(main src/main.cpp
    src/video_reader.cpp
    src/video_writer.cpp
    src/inference_engine.cpp
)
set_property(TARGET main PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
# Link libraries
target_link_libraries(main
    nvinfer_10
    nvonnxparser_10
    cudart
    onnxruntime
    gstreamer-1.0
    gstapp-1.0
    gstbase-1.0
    gobject-2.0
    glib-2.0
)


# add_executable(convert_to_engine src/convert_to_engine.cpp)
# target_link_libraries(convert_to_engine
#     nvinfer_10
#     nvonnxparser_10
#     cuda
#     cudart
# )

# add_executable(trt_cuda_graph src/trt_cuda_graph.cpp)
# target_link_libraries(trt_cuda_graph
#     nvinfer_10
#     nvonnxparser_10
#     cuda
#     cudart
# )

############################################################################




# Executable for reading and rendering video using Gstreamer
# add_executable(app src/Video_Read_and_Render.cpp)

# Link the GStreamer libraries to the executable
# target_link_libraries(app
#     gstreamer-1.0
#     gstapp-1.0
#     gstbase-1.0
#     gobject-2.0
#     glib-2.0
# )