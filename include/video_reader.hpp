#pragma once
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>

class VideoReader{
    public:
        VideoReader(const std::string &source);
        ~VideoReader();
        bool readFrame(uint8_t** frame, int &width, int &height);

        int getWidth() const { return width; }
        int getHeight() const { return height; }
        double getFPS() const { return fps; }
    private:
        GstElement *pipeline;
        GstAppSink *appsrc;
        int width;
        int height;
        double fps;
        bool initialized;
        void buildPipeline(const std::string &source);
        bool getVideoInfo();
};