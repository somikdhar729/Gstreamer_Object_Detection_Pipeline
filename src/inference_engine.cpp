#include "inference_engine.hpp"

#include <iostream>
#include <cstring>
#include <fstream>
#include <array>

// TensorRT 
#include <NvInfer.h>
#include <cuda_runtime_api.h>

// ONNX Runtime
#include <onnxruntime_cxx_api.h>

using namespace nvinfer1;

// Logger
class Logger : public ILogger{
    public:
        void log(Severity severity, const char* msg) noexcept override{
            if(severity <= Severity::kWARNING){
                std::cout << "[Inference Engine] " << msg << std::endl;
            }
        }
} logger;

// Constructor: Load model and initialize inference engine
InferenceEngine::InferenceEngine(const std::string& model_path): 
    ort_env_(ORT_LOGGING_LEVEL_WARNING, "InferenceEngine"),
    session_options(),
    ort_session_(nullptr),
    input_buffer_(3 * 640 * 640) 
{
    if(model_path.find(".engine") != std::string::npos){
        backend_ = BackendType::TENSORRT;
        std::cout << "Initializing TensorRT engine..." << std::endl;
        initTensorRT(model_path);
    }
    else if(model_path.find(".onnx") != std::string::npos){
        backend_ = BackendType::ONNX;
        std::cout << "Initializing ONNX Runtime..." << std::endl;
        initONNX(model_path);
    }
    else{
        throw std::runtime_error("Unsupported model format. Use .onnx or .engine");
    }
}

// Destructor: Clean up resources
InferenceEngine::~InferenceEngine(){
    if(backend_ == BackendType::TENSORRT){
        cudaFree(device_input_);
        cudaFree(device_output_);
        delete context_;
        delete engine_;
        delete runtime_;
    }
    
    delete[] host_output_;
}

// ONNX Init
void InferenceEngine::initONNX(const std::string& path){
    
    session_options.SetIntraOpNumThreads(1); // 1 CPU thread per operation. For deterministic results.
    std::wstring wpath(path.begin(), path.end());

    ort_session_ = Ort::Session(ort_env_, wpath.c_str(), session_options);
}

// TensorRT Init
void InferenceEngine::initTensorRT(const std::string& path){
    // Load engine file
    std::ifstream file(path, std::ios::binary);
    if(!file){
        throw std::runtime_error("Failed to open engine file: " + path);
    }
    file.seekg(0, file.end);
    size_t engine_size = file.tellg();
    file.seekg(0, file.beg);

    std::vector<char> engine_data(engine_size);
    file.read(engine_data.data(), engine_size);

    runtime_ = createInferRuntime(logger);
    engine_ = runtime_->deserializeCudaEngine(engine_data.data(), engine_size);   
    context_ = engine_->createExecutionContext();

    input_size_ = 1 * 3 * 640 * 640 * sizeof(float); // Assuming input is (1,3,640,640)
    output_size_ = 1 * 300 * 6 * sizeof(float); // Assuming output is (1,300,6)

    cudaMalloc(&device_input_, input_size_);
    cudaMalloc(&device_output_, output_size_);

    host_output_ = new float[1 * 300 * 6]; // Adjust size based on actual output
}

// Preprocess 
void InferenceEngine::preprocess(uint8_t* frame, int width, int height, float* input){
    const int target_width = 640;
    const int target_height = 640;
    
    // 1. Scale(Keep Aspect ratio)
    float scale = std::min(static_cast<float>(target_width) / width, static_cast<float>(target_height) / height);
    int new_width = static_cast<int>(width * scale);
    int new_height = static_cast<int>(height * scale);
    int pad_x = (target_width - new_width) / 2;
    int pad_y = (target_height - new_height) / 2;

    // 2. Fill with padding value(114)
    float pad_value = 114.0f / 255.0f; // Normalize padding value to [0,1]
    for(int i = 0; i < target_width * target_height * 3; ++i){
        input[i] = pad_value;
    }

    // 3. Bilinear resize + copy
    for(int dy = 0; dy < new_height; ++dy){
        for(int dx = 0; dx < new_width; ++dx){
            // Map destination -> source
            float sx = (dx + 0.5f) / scale - 0.5f; // Add 0.5 for pixel center alignment
            float sy = (dy + 0.5f) / scale - 0.5f;

            // Clamp to valid range
            sx = std::max(0.0f, std::min(sx, width - 1.0f));
            sy = std::max(0.0f, std::min(sy, height - 1.0f));

            int x0 = static_cast<int>(sx);
            int y0 = static_cast<int>(sy);
            int x1 = std::min(x0 + 1, width - 1);
            int y1 = std::min(y0 + 1, height - 1);

            float wx = sx - x0;
            float wy = sy - y0;

            int dst_x = dx + pad_x;
            int dst_y = dy + pad_y;

            for(int c = 0; c < 3; ++c){
                // Fetch 4 neighbours
                float v00 = frame[(y0 * width + x0) * 3 + c];
                float v01 = frame[(y0 * width + x1) * 3 + c];
                float v10 = frame[(y1 * width + x0) * 3 + c];
                float v11 = frame[(y1 * width + x1) * 3 + c];

                // Bilinear interpolation
                float value = (1 - wx) * (1 - wy) * v00 + 
                              wx * (1 - wy) * v01 + 
                              (1 - wx) * wy * v10 + 
                              wx * wy * v11;

                // Normalize to [0,1]
                input[c * target_width * target_height + dst_y * target_width + dst_x] = value / 255.0f;
            }
        }
    }
}

// Postprocess
std::vector<Detection> InferenceEngine::postprocess(float* output, int orig_w, int orig_h){
    std::vector<Detection> detections;
    const int target_width = 640;
    const int target_height = 640;
    
    // Same values used in preprocess
    float scale = std::min(static_cast<float>(target_width) / orig_w, static_cast<float>(target_height) / orig_h);
    int new_width = static_cast<int>(orig_w * scale);
    int new_height = static_cast<int>(orig_h * scale);
    int pad_x = (target_width - new_width) / 2;
    int pad_y = (target_height - new_height) / 2;

    detections.reserve(300); // Assuming max 300 detections
    for(int i = 0; i < 300; ++i){
        float conf = output[i * 6 + 4];
        if(conf < 0.5f) continue; // Confidence threshold
        float x1 = output[i * 6 + 0];
        float y1 = output[i * 6 + 1];
        float x2 = output[i * 6 + 2];
        float y2 = output[i * 6 + 3];

        // 1. Remove padding
        x1 -= pad_x;
        y1 -= pad_y;
        x2 -= pad_x;
        y2 -= pad_y;
        // 2. Scale back to original size
        x1 /= scale;
        y1 /= scale;
        x2 /= scale;
        y2 /= scale;

        // Clamp to original image size
        x1 = std::max(0.0f, std::min(x1, static_cast<float>(orig_w - 1))); // -1 to avoid out of bounds
        y1 = std::max(0.0f, std::min(y1, static_cast<float>(orig_h - 1)));
        x2 = std::max(0.0f, std::min(x2, static_cast<float>(orig_w - 1)));
        y2 = std::max(0.0f, std::min(y2, static_cast<float>(orig_h - 1)));

        Detection det;
        det.x1 = x1;
        det.y1 = y1;
        det.x2 = x2;
        det.y2 = y2;
        det.confidence = conf;
        det.class_id = static_cast<int>(output[i * 6 + 5]);
        detections.push_back(det);
    }
    return detections;

}

// ---------------- DETECT ----------------
std::vector<Detection> InferenceEngine::detect(uint8_t* frame, int width, int height){

    if(backend_ == BackendType::ONNX){

        // 1. Preprocess
        preprocess(const_cast<uint8_t*>(frame), width, height, input_buffer_.data());

        // 2. Create memory info
        static Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault); // Create memory info once and reuse for all inferences
        
        // 3. Create input tensor
        std::array<int64_t, 4> input_shape{1,3,640,640};
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_buffer_.data(), input_buffer_.size(), input_shape.data(), input_shape.size());
        
        // 4. Get input and output names
        auto input_names = ort_session_.GetInputNameAllocated(0, Ort::AllocatorWithDefaultOptions());
        auto output_names = ort_session_.GetOutputNameAllocated(0, Ort::AllocatorWithDefaultOptions());

        const char *input_name[] = {input_names.get()};
        const char *output_name[] = {output_names.get()};

        // 5. Run inference
        auto output_tensors = ort_session_.Run(
            Ort::RunOptions{nullptr}, 
            input_name, &input_tensor, 1, 
            output_name, 1); // Assuming single input and single output
        
        // 6. Extract output and postprocess
        float* output = output_tensors[0].GetTensorMutableData<float>();

        
        return postprocess(output, width, height);
    }

    else{ // TensorRT

        std::vector<float> input(3 * 640 * 640);
        preprocess(frame, width, height, input.data());

        cudaMemcpy(device_input_, input.data(), input_size_, cudaMemcpyHostToDevice);

        context_->setTensorAddress("images", device_input_);
        context_->setTensorAddress("output", device_output_);
        context_->enqueueV3(0);

        cudaMemcpy(host_output_, device_output_, output_size_, cudaMemcpyDeviceToHost);

        return postprocess(host_output_, width, height);
    }
}