/*
Full GPU video inference pipeline
*/
#include <iostream>
#include "video_reader.hpp"
#include "video_writer.hpp"
#include "inference_engine.hpp"
#include "timer.hpp"

#include <vector>
#include <stdint.h>

struct Detection{
    float x1, y1, x2, y2;
    float confidence;
    int class_id;
};

void drawBBox(uint8_t* frame, int width, int height, const std::vector<Detection> &detections){
    for(const auto &det : detections){
        // Clamp coordinates to frame boundaries
        int x1 = std::max(0, static_cast<int>(det.x1 * width));
        int y1 = std::max(0, static_cast<int>(det.y1 * height));
        int x2 = std::min(width - 1, static_cast<int>(det.x2 * width));
        int y2 = std::min(height - 1, static_cast<int>(det.y2 * height));

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
        std::cerr << "Usage: " << argv[0] << " <input_video>  <model_path> <output_video>" << std::endl;
        return -1;
    }

    std::string input_video = argv[1];
    std::string model_path = argv[2];
    std::string output_video = argv[3];

    // Intialize video reader, inference engine, and video writer
    VideoReader reader(input_video);
    InferenceEngine engine(model_path);
    VideoWriter writer(output_video, reader.getWidth(), reader.getHeight(), reader.getFPS());

    // Timers for measuring performance
    Timer total_timer;
    Timer inference_timer;  
    Timer decode_timer;
    Timer encode_timer;

    int frame_count = 0;
    uint8_t* frame = nullptr;
    int width, height;

    while(true){
        // Decode frame
        decode_timer.start();
        bool success = reader.readFrame(&frame, width, height);
        decode_timer.stop();

        if(!success) break;

        // Inference
        inference_timer.start();
        std::vector<Detection> detections = engine.detect(frame, width, height);
        inference_timer.stop();

        // Draw bounding boxes
        drawBBox(frame, width, height, detections);

        // Encode frame
        encode_timer.start();
        writer.writeFrame(frame, width, height);
        encode_timer.stop();

        frame_count++;
    }

    double total_time = decode_timer.getTotalTime() + inference_timer.getTotalTime() + encode_timer.getTotalTime();
    std::cout << "Processed " << frame_count << " frames in " << total_time << " seconds." << std::endl;
    std::cout << "Average FPS: " << frame_count / total_time << std::endl;

    return 0;

}