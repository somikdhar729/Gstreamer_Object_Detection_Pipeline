#include "video_writer.hpp"
#include <iostream>
#include <cstring>  // for memcpy


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
        GstBus* bus = gst_element_get_bus(pipeline);

        // Wait for EOS or ERROR
        gst_bus_timed_pop_filtered(
            bus,
            GST_CLOCK_TIME_NONE,
            static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR)
        );

        gst_object_unref(bus);

        // ✅ ADD THIS HERE (flush pipeline + finalize file)
        gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        // Now safely stop pipeline
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void VideoWriter::buildPipeline(const std::string &output){
    int fps_num = static_cast<int>(fps * 1000);
    int fps_den = 1000;
    std::string caps_str = "video/x-raw,format=RGB,width=" + 
                            std::to_string(width) + 
                            ",height=" + std::to_string(height) + 
                            ",framerate=" + 
                            std::to_string(fps_num) + "/" + std::to_string(fps_den);
    
    std::string pipeline_str =
                        "appsrc name=appsrc ! "
                        "videoconvert ! "
                        "x264enc bitrate=5000 speed-preset=ultrafast tune=zerolatency ! "
                        "h264parse ! "
                        "mp4mux ! "
                        "filesink location=\"" + output + "\"";

    GError* error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if(error){
        std::cerr << "Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        initialized = false;
        return;
    }
    
    
    GstElement* elem = gst_bin_get_by_name(GST_BIN(pipeline), "appsrc");
    appsrc = GST_APP_SRC(elem);
    if(!appsrc){
        std::cerr << "Error getting appsrc element from pipeline." << std::endl;
        gst_object_unref(pipeline);
        initialized = false;
        return;
    }

    // Configure appsrc
    GstCaps* caps = gst_caps_from_string(caps_str.c_str());

    g_object_set(G_OBJECT(appsrc),
        "caps", caps,
        "format", GST_FORMAT_TIME,
        "is-live", FALSE,
        "block", TRUE,
        NULL);

    gst_caps_unref(caps);

    initialized = true;
    
}


void VideoWriter::writeFrame(uint8_t* frame, int width, int height){

    if(!initialized || !appsrc){
        std::cerr << "VideoWriter not initialized properly\n";
        return;
    }

    int size = width * height * 3;  // RGB

    // Allocate GStreamer buffer
    GstBuffer* buffer = gst_buffer_new_allocate(NULL, size, NULL);
    if(!buffer){
        std::cerr << "Failed to allocate GstBuffer\n";
        return;
    }

    // Map buffer memory
    GstMapInfo map;
    if(!gst_buffer_map(buffer, &map, GST_MAP_WRITE)){
        std::cerr << "Failed to map GstBuffer\n";
        gst_buffer_unref(buffer);
        return;
    }

    // Copy frame data into buffer
    std::memcpy(map.data, frame, size);

    gst_buffer_unmap(buffer, &map);

    // Set timestamps (VERY IMPORTANT for smooth video)
    static uint64_t frame_count = 0;

    GstClockTime pts = gst_util_uint64_scale(frame_count, GST_SECOND, static_cast<int>(fps));

    GST_BUFFER_PTS(buffer) = pts;
    GST_BUFFER_DTS(buffer) = pts;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, static_cast<int>(fps));

    frame_count++;

    // Push buffer into pipeline
    GstFlowReturn ret = gst_app_src_push_buffer(appsrc, buffer);

    if(ret != GST_FLOW_OK){
        std::cerr << "Failed to push buffer to appsrc\n";
    }
}
