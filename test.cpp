#include <onnxruntime_cxx_api.h>
#include <iostream>

void printProviders() {
    auto providers = Ort::GetAvailableProviders();
    std::cout << "Available providers:\n";
    for (const auto& p : providers) {
        std::cout << "  " << p << "\n";
    }
}