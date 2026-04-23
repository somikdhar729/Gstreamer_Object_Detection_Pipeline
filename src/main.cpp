#include "video_reader.hpp"
#include "video_writer.hpp"
#include "inference_engine.hpp"
#include "timer.hpp"

#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>

void drawBBox(uint8_t* frame, int width, int height, const std::vector<Detection> &detections){
    for(const auto &det : detections){
        // Clamp coordinates to frame boundaries
        int x1 = std::max(0, static_cast<int>(det.x1));
        int y1 = std::max(0, static_cast<int>(det.y1));
        int x2 = std::min(width - 1, static_cast<int>(det.x2));
        int y2 = std::min(height - 1, static_cast<int>(det.y2));

        if(x1 >= x2 || y1 >= y2) continue; // Invalid box, skip

        // Draw bounding box (simple red rectangle)
        const int thickness = 2; // Thickness of the box

        for(int t = 0; t < thickness; ++t){
            // Top and bottom lines
            for(int x = x1; x <= x2; ++x){
                // Top line
                if(y1 + t < height){
                    frame[(y1 + t) * width * 3 + x * 3] = 255;     // R
                    frame[(y1 + t) * width * 3 + x * 3 + 1] = 0;   // G
                    frame[(y1 + t) * width * 3 + x * 3 + 2] = 0;   // B
                }
                // Bottom line
                if(y2 - t >= 0){
                    frame[(y2 - t) * width * 3 + x * 3] = 255;     // R
                    frame[(y2 - t) * width * 3 + x * 3 + 1] = 0;   // G
                    frame[(y2 - t) * width * 3 + x * 3 + 2] = 0;   // B
                }
            }
            // Left and right lines
            for(int y = y1; y <= y2; ++y){
                // Left line
                if(x1 + t < width){
                    frame[y * width * 3 + (x1 + t) * 3] = 255;     // R
                    frame[y * width * 3 + (x1 + t) * 3 + 1] = 0;   // G
                    frame[y * width * 3 + (x1 + t) * 3 + 2] = 0;   // B
                }
                // Right line
                if(x2 - t >= 0){
                    frame[y * width * 3 + (x2 - t) * 3] = 255;     // R
                    frame[y * width * 3 + (x2 - t) * 3 + 1] = 0;   // G
                    frame[y * width * 3 + (x2 - t) * 3 + 2] = 0;   // B
                }
            }
        }

    }
}


int main(int argc, char** argv){
    if(argc != 4){
        std::cerr << "Usage: " << argv[0] << " <input_video_path>  <model_path(.onnx/.engine)> <output_video_path>" << std::endl;
        return -1;
    }
    
    // Parse command line arguments
    // Modern C++ style allow us to use forward slashes in file paths on Windows, but we will replace backslashes just in case
    std::string input_video_path = argv[1];
    std::replace(input_video_path.begin(), input_video_path.end(), '\\', '/');

    std::string model_path = argv[2];
    std::replace(model_path.begin(), model_path.end(), '\\', '/');

    std::string output_video_path = argv[3];
    std::replace(output_video_path.begin(), output_video_path.end(), '\\', '/');
    
    // Detect backend
    bool is_engine = model_path.find(".engine") != std::string::npos;
    bool is_onnx = model_path.find(".onnx") != std::string::npos;
    if(!is_engine && !is_onnx){
        std::cerr << "Error: Model path must end with .onnx or .engine" << std::endl;
        return -1;
    }
    std::cout << "Using backend: " << (is_engine ? "TensorRT Engine(GPU)" : "ONNX Model(CPU)") << std::endl;
    
    // Intialize video reader, inference engine, and video writer
    VideoReader reader(input_video_path);
    InferenceEngine engine(model_path);
    VideoWriter writer(output_video_path, reader.getWidth(), reader.getHeight(), reader.getFPS());

    // Timers for measuring performance
    Timer total_timer;
    Timer inference_timer;  
    Timer decode_timer;
    Timer encode_timer;
    Timer draw_timer;

    int frame_count = 0;
    const int warmup_frames = 10; // Number of frames to skip for warmup
    uint8_t* frame = nullptr;
    int width=0, height=0;
    int warmup = 10;
    int processed = 0;

    // total_timer.start();
    while(true){
        // Decode frame
        if(processed >= warmup_frames){
            decode_timer.start();
        }
        bool success = reader.readFrame(frame, width, height);

        if(processed >= warmup_frames){
            decode_timer.stop();
        }

        if (!success) break;
        // Start total timer after warmup
        if(processed == warmup_frames){
            total_timer.reset();
            total_timer.start();
        }
        // Inference
        if(processed >= warmup_frames){
            inference_timer.start();
        }
        std::vector<Detection> detections = engine.detect(frame, width, height);
        if(processed >= warmup_frames){
            inference_timer.stop();
        }

        // Draw bounding boxes
        if(processed >= warmup_frames){
            draw_timer.start();
        }
        drawBBox(frame, width, height, detections);
        if(processed >= warmup_frames){
            draw_timer.stop();
        }

        // Encode frame
        if(processed >= warmup_frames){
            encode_timer.start();
        }
        writer.writeFrame(frame, width, height);
        if(processed >= warmup_frames){
            encode_timer.stop();
        }

        if(processed >= warmup_frames){
            frame_count++;
        }
        processed++;}
    total_timer.stop();
    double total_time = total_timer.getTotalTime();

    std::cout << "\n==== Performance Breakdown ====\n";
    std::cout << "Frames (measured): " << frame_count << "\n";
    std::cout << "Decode:    " << decode_timer.getTotalTime()    / frame_count<< " s\n";
    std::cout << "Inference: " << inference_timer.getTotalTime() / frame_count<< " s\n";
    std::cout << "Draw:      " << draw_timer.getTotalTime()      / frame_count<< " s\n";
    std::cout << "Encode:    " << encode_timer.getTotalTime()    / frame_count<< " s\n";
    std::cout << "Total:     " << total_time << " s\n";
    std::cout << "FPS:       " << (frame_count / total_time) << "\n";

    return 0;

}