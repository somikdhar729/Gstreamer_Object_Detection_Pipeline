#include <NvInfer.h>
#include <cuda_runtime_api.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace nvinfer1;

// Logger
class Logger : public ILogger{
    public:
        void log(Severity severity, const char* msg) noexcept override {
            if (severity <= Severity::kINFO) {
                std::cout << msg << std::endl;
            }
        }
} logger;

// Load TensorRT engine from file
ICudaEngine* loadEngine(const std::string& path){
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening engine file: " << path << std::endl;
        return nullptr;
    }
    file.seekg(0, file.end);
    size_t size = file.tellg();
    file.seekg(0, file.beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    IRuntime* runtime = createInferRuntime(logger);
    return runtime->deserializeCudaEngine(buffer.data(), size);
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <engine_path>\n";
        return -1;
    }
    std::string enginePath = argv[1];
    ICudaEngine* engine = loadEngine(enginePath);
    if (!engine) {
        std::cerr << "Failed to load engine\n";
        return -1;
    }
    IExecutionContext* context = engine->createExecutionContext();
    if (!context) {
        std::cerr << "Failed to create execution context\n";
        return -1;
    }


    size_t inputSize = 1 * 3 * 640 * 640 * sizeof(float); 
    size_t outputSize = 1 * 300 * 6 * sizeof(float);

    // Allocate host memory
    float* hostInput, *hostOutput;
    cudaHostAlloc((void**)&hostInput, inputSize, cudaHostAllocDefault);
    cudaHostAlloc((void**)&hostOutput, outputSize, cudaHostAllocDefault);
    // Allocate device memory
    void* deviceInput, *deviceOutput;
    cudaMalloc(&deviceInput, inputSize);
    cudaMalloc(&deviceOutput, outputSize);

    // Create CUDA stream
    cudaStream_t stream;
    cudaStreamCreate(&stream);

    // CUDA Graph capture
    cudaGraph_t graph;
    cudaGraphExec_t graphExec;
    cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal);
    // H2D
    cudaMemcpyAsync(deviceInput, hostInput, inputSize, cudaMemcpyHostToDevice, stream);
    // Inference
    context->setTensorAddress("images", deviceInput);
    context->setTensorAddress("output", deviceOutput);
    context->enqueueV3(stream);
    // D2H
    cudaMemcpyAsync(hostOutput, deviceOutput, outputSize, cudaMemcpyDeviceToHost, stream);
    cudaStreamEndCapture(stream, &graph);
    cudaGraphInstantiate(&graphExec, graph, 0);
    
    std::cout<<"CUDA Graph created" <<std::endl;

    // Inference loop
    for (int i = 0; i < 100; ++i) {
        for(int j = 0; j < 10;j++){
            hostInput[j] = static_cast<float>(i * 10 + j); // Dummy data
        }
        cudaGraphLaunch(graphExec, stream);
        cudaStreamSynchronize(stream);
        std::cout << "Inference " << i + 1 << " completed\n";
    }

    cudaFree(deviceInput);
    cudaFree(deviceOutput);
    cudaFreeHost(hostInput);
    cudaFreeHost(hostOutput);
    delete context;
    delete engine;
    cudaStreamDestroy(stream);

    return 0;
}