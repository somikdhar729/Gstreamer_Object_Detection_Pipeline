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

        // flush pipeline + finalize file
        gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

        // Safely stop pipeline
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
}

void VideoWriter::buildPipeline(const std::string &output){
    int fps_num = static_cast<int>(fps * 1000);
    int fps_den = 1000;
    std::string caps_str = "video/x-raw,format=RGB,width=" + std::to_string(width) + ",height=" + std::to_string(height) + 
                            ",framerate=" + std::to_string(fps_num) + "/" + std::to_string(fps_den);
    
    std::string pipeline_str = "appsrc name=appsrc ! videoconvert ! "
                                "x264enc bitrate=5000 speed-preset=ultrafast tune=zerolatency ! "
                                "h264parse ! mp4mux ! "
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
        "caps", caps, // Set the caps to match our input frame format
        "format", GST_FORMAT_TIME, // Use time-based format for timestamps
        "is-live", FALSE, // Not a live source
        "block", TRUE, // Important for flow control
        NULL); // NULL terminator for g_object_set

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
    GstMapInfo map; // For writing, we use GST_MAP_WRITE. map will contain a pointer to the buffer's memory and its size after mapping.
    if(!gst_buffer_map(buffer, &map, GST_MAP_WRITE)){
        std::cerr << "Failed to map GstBuffer\n";
        gst_buffer_unref(buffer);
        return;
    }

    // Copy frame data into buffer
    std::memcpy(map.data, frame, size); // Copy the raw RGB frame data into the mapped buffer memory. It is important to ensure that the size of the data being copied matches the size of the buffer to avoid memory issues.

    gst_buffer_unmap(buffer, &map); // Unmap the buffer after writing. This is important to ensure that the data is properly flushed to the buffer and that we don't have any memory leaks.

    // Set timestamps (VERY IMPORTANT for smooth video)
    static uint64_t frame_count = 0;

    GstClockTime pts = gst_util_uint64_scale(frame_count, GST_SECOND, static_cast<int>(fps));// Calculate the presentation timestamp (PTS) for the current frame based on the frame count and the frames per second (fps). This ensures that each frame is displayed at the correct time in the video stream.

    GST_BUFFER_PTS(buffer) = pts;// Set the PTS of the buffer to the calculated timestamp. This is crucial for maintaining proper timing in the video stream, ensuring that frames are displayed in the correct order and at the correct intervals.
    GST_BUFFER_DTS(buffer) = pts;// Set the decoding timestamp (DTS) to the same value as PTS. In many cases, especially for simple video streams, PTS and DTS can be the same. However, in more complex scenarios with B-frames or reordering, DTS may differ from PTS. Setting both to the same value is a common practice for straightforward encoding pipelines.

    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, static_cast<int>(fps));

    frame_count++;

    // Push buffer into pipeline
    GstFlowReturn ret = gst_app_src_push_buffer(appsrc, buffer);// Push the buffer containing the frame data into the appsrc element of the GStreamer pipeline. This is how we feed our raw video frames into the encoding and muxing process defined in our pipeline.

    if(ret != GST_FLOW_OK){
        std::cerr << "Failed to push buffer to appsrc\n";
    }
}
