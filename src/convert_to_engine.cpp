#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>
#include <iostream>
#include <fstream>

using namespace nvinfer1;

class Logger : public ILogger{
    public:
        void log(Severity severity, const char* msg) noexcept override{
            if(severity <= Severity::kINFO){
                std::cout << msg << std::endl;
            }
        }
}logger;

// Save engine to file
void saveEngine(ICudaEngine* engine, const std::string& path){
    IHostMemory* serialized = engine->serialize();
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << path << std::endl;
        delete serialized;
        return;
    }
    // std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(serialized->data()), serialized->size());
    delete serialized;
}

int main(int argc, char** argv){
    if(argc < 3){
        std::cerr << "Usage: " << argv[0] << " <onnx_model_path> <engine_output_path>" << std::endl;
        return -1;
    }
    std::string onnx_path = argv[1];
    std::string engine_path = argv[2];

    // Create builder and network
    IBuilder* builder = createInferBuilder(logger);
    uint32_t flag = 1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    INetworkDefinition* network = builder->createNetworkV2(flag);

    // Create ONNX parser
    auto parser = nvonnxparser::createParser(*network, logger);

    if(!parser->parseFromFile(onnx_path.c_str(), static_cast<int>(ILogger::Severity::kINFO))){
        std::cerr << "Failed to parse ONNX model." << std::endl;
        return -1;
    }

    std::cout<< "Building TensorRT engine..." << std::endl;
    // Create config
    IBuilderConfig* config = builder->createBuilderConfig();
    config->setMemoryPoolLimit(MemoryPoolType::kWORKSPACE, 4ULL * 1024 * 1024 * 1024); // 4GB workspace
    config->setProfilingVerbosity(nvinfer1::ProfilingVerbosity::kDETAILED);
    // Enable FP16 if supported
    if(builder->platformHasFastFp16()){
        config->setFlag(BuilderFlag::kFP16);
        std::cout << "FP16 supported and enabled." << std::endl;
    }

    // Build engine
    ICudaEngine* engine = builder->buildEngineWithConfig(*network, *config);

    if(!engine){
        std::cerr << "Failed to build engine." << std::endl;
        return -1;
    }

    saveEngine(engine, engine_path);
    delete engine;
    delete network;
    delete parser;
    delete config;
    delete builder;

    return 0;
}
