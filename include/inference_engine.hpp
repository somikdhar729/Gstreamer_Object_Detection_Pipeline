#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <onnxruntime_cxx_api.h>


// Forward declaration of InferenceEngine class
namespace nvinfer1{
    class IRuntime;
    class ICudaEngine;
    class IExecutionContext;
}

// Detection structure to hold bounding box and confidence information
struct Detection{
    float x1, y1, x2, y2;
    float confidence;
    int class_id;
};

class InferenceEngine {
    public:
        InferenceEngine(const std::string& model_path); // Constructor to initialize the inference engine with the given model path
        ~InferenceEngine(); // Destructor to clean up resources

        // Detect objects in the input frame and return a list of detections
        void preprocess(uint8_t* frame, int width, int height, float* input);
        std::vector<Detection> postprocess(float* output, int orig_w, int orig_h);
        float* detect(const float* input, int width, int height);

    private:
        // Backend type enumeration to differentiate between ONNX Runtime and TensorRT
        enum BackendType {
                ONNX,
                TENSORRT
            };
        BackendType backend_;

        // ONNX 
        Ort::Env ort_env_; // ONNX Runtime environment
        Ort::Session ort_session_; // ONNX Runtime session
        Ort::SessionOptions session_options; // ONNX Runtime session options - can be configured for performance tuning
        
        std::vector<float> input_buffer_;
        void initONNX(const std::string& model_path);
        

        // TensorRT
        void initTensorRT(const std::string& model_path);
        nvinfer1::IRuntime* runtime_;
        nvinfer1::ICudaEngine* engine_;
        nvinfer1::IExecutionContext* context_;


        void* device_input_ = nullptr;
        void* device_output_ = nullptr;
        float* host_input_ = nullptr;
        float* host_output_ = nullptr;

        size_t input_size_;
        size_t output_size_;

        // void preprocess(uint8_t* frame, int width, int height, float* input);
        // std::vector<Detection> postprocess(float* output, int orig_w, int orig_h);
};