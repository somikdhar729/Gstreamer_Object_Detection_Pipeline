#include "video_writer.hpp"
#include <iostream>

VideoWriter::VideoWriter(const std::string &output, int width, int height, double fps){
    this->width = width;
    this->height = height;
    this->fps = fps;
    this->initialized = false;
    buildPipeline(output);
    if(initialized){
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }
}

VideoWriter::~VideoWriter(){
    if(appsrc){
        gst_app_src_end_of_stream(GST_APP_SRC(appsrc));
    }

    if(pipeline){
        GstBus* bus = gst_element_get_bus(pipeline); // Get the bus from the pipeline
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR)); // Wait for EOS or ERROR message
        gst_object_unref(bus); // Unref the bus after use
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void VideoWriter::buildPipeline(const std::string &output){
    std::string caps_str = "video/x-raw,format=RGB,width=" + std::to_string(width) + ",height=" + std::to_string(height) + ",framerate=" + std::to_string(static_cast<int>(fps)) + "/1";
    
    std::string pipeline_str = "appsrc name=appsrc ! videoconvert ! nvh264enc bitrate=5000 speed-preset=ultrafast ! mp4mux ! filesink location=" + output;

    GError* error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if(error){
        std::cerr << "Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        initialized = false;
        return;
    
    
    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "src");
    if(!appsrc){
        std::cerr << "Error getting appsrc element from pipeline." << std::endl;
        gst_object_unref(pipeline);
        initialized = false;
        return;
    }

    // Configure appsrc
    g_object_set(G_OBJECT(appsrc), "str")
}

