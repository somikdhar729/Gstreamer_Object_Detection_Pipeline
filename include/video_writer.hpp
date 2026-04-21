#pragma once
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <string>

class VideoWriter{
    public:
        VideoWriter(const std::string &output, int width, int height, double fps);
        ~VideoWriter();
        void writeFrame(uint8_t* frame, int width, int height);

    private:
        GstElement *pipeline;
        GstAppSrc *appsrc;
        bool initialized;
        void buildPipeline(const std::string &output);
        int width;
        int height;
        double fps;
};