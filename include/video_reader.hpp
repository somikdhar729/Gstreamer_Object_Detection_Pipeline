#pragma once
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

class VideoReader{
    public:
        VideoReader(const std::string &source);
        ~VideoReader();
        bool readFrame(uint8_t*& frame, int &width, int &height);

        int getWidth() const { return width; }
        int getHeight() const { return height; }
        double getFPS() const { return fps; }

    private:
        GstElement *pipeline;
        GstAppSink *appsink;
        int width;
        int height;
        double fps;
        std::vector<uint8_t> frame_buffer; // Buffer to hold the current frame data
        bool initialized;
        void buildPipeline(const std::string &source);
        bool getVideoInfo();
};