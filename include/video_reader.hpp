#pragma once
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string>
#include <vector>
#include <iostream>

class VideoReader{
    public:
        VideoReader(const std::string &source);
        ~VideoReader();
        bool readFrame(uint8_t*& frame, int &width, int &height);

        int getWidth() const {return width;}
        int getHeight() const {return height;}
        double getFPS() const {return fps;}

    private:
        GstElement *pipeline; // GStreamer pipeline element to read and decode video frames. 
        GstAppSink *appsink; // GStreamer appsink element to pull decoded video frames from the pipeline. Appsink allows us to access the raw video frames in our application after they have been decoded by the pipeline.
        int width;
        int height;
        double fps;
        std::vector<uint8_t> frame_buffer; // Buffer to hold the current frame data
        bool initialized;
        void buildPipeline(const std::string &source);
        bool getVideoInfo();
};