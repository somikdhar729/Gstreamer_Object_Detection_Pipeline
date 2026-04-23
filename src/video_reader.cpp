#include "video_reader.hpp"

VideoReader::VideoReader(const std::string &source) : pipeline(nullptr), appsink(nullptr), initialized(false){
    gst_init(nullptr, nullptr); // Initialize GStreamer. This is necessary before using any GStreamer functions. It sets up the internal data structures and prepares the library for use.
    buildPipeline(source);
    if(initialized){
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        // WAIT until ready
        gst_element_get_state(pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);

        getVideoInfo();
    }
}

VideoReader::~VideoReader(){
    if(pipeline){
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    if(appsink){
        gst_object_unref(appsink);
    }
}

void VideoReader::buildPipeline(const std::string &source){
    std::string pipeline_desc = "filesrc location=\"" + source + "\" ! "
                                "qtdemux ! "
                                "h264parse ! "
                                "avdec_h264 ! "
                                "videoconvert ! "
                                "video/x-raw,format=RGB ! "
                                "appsink name=sink max-buffers=1 drop=false sync=false";

    GError *error = nullptr; // GError structure to hold error information if pipeline creation fails
    pipeline = gst_parse_launch(pipeline_desc.c_str(), &error); // Create the GStreamer pipeline from the description string. If there is an error during pipeline creation, the error variable will be set with details about the failure

    if(!pipeline){
        std::cerr << "Failed to create pipeline: " << error->message << std::endl;
        g_error_free(error);
        return;
    }
    
    appsink = GST_APP_SINK(gst_bin_get_by_name(GST_BIN(pipeline), "sink")); // Get the appsink element from the pipeline by name. This allows us to access the decoded video frames in our application. If we fail to get the appsink, we need to clean up the pipeline and return

    if(!appsink){
        std::cerr << "Failed to get appsink from pipeline" << std::endl;
        gst_object_unref(pipeline);
        pipeline = nullptr;
        return;
    }
    initialized = true;
}

bool VideoReader::getVideoInfo(){
    // Wait for the first frame to get video info
    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 5 * GST_SECOND); // Wait up to 5 seconds
    if (!sample) {
        std::cerr << "Failed to pull sample\n";
        return false;
    }

    GstCaps *caps = gst_sample_get_caps(sample); // Get the capabilities of the sample
    GstStructure *structure = gst_caps_get_structure(caps, 0); // Get the first structure. Here structure is the description of the video format and contains the width, height, and framerate of the video

    gst_structure_get_int(structure, "width", &width); // Get the width of the video
    gst_structure_get_int(structure, "height", &height); // Get the height of the video

    gint fps_num, fps_den; // Use gint instead of int for framerate numerator and denominator because gst_structure_get_fraction expects gint pointers
    gst_structure_get_fraction(structure, "framerate", &fps_num, &fps_den); // Get the framerate of the video as a fraction (numerator and denominator)
    fps = static_cast<double>(fps_num) / fps_den; // Calculate the frames per second

    // Unref the sample to free resources
    gst_sample_unref(sample);
    std::cout << "Video Info - Width: " << width << ", Height: " << height << ", FPS: " << fps << std::endl;
    return true;    
}

bool VideoReader::readFrame(uint8_t*& frame, int &width, int &height){
    if(!initialized) return false;

    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), GST_SECOND); // Wait up to 1 second for a frame
    if(!sample){
        if (gst_app_sink_is_eos(appsink)) {
            std::cout << "End of stream reached\n";
            return false;  // real stop condition
        }
        std::cerr << "Failed to pull sample" << std::endl;
        return false;
    }
    GstBuffer *buffer = gst_sample_get_buffer(sample); // Get the buffer containing the frame data
    GstMapInfo map; // Structure to hold the mapped buffer data because gst_buffer_map requires a GstMapInfo pointer. Utility is to access the raw data of the buffer
    if(!gst_buffer_map(buffer, &map, GST_MAP_READ)){ // Map the buffer for reading. If mapping fails, return false
        std::cerr << "Failed to map buffer" << std::endl;
        gst_sample_unref(sample); // Unref the sample to free resources
        return false;
    }
    width = this->width; // Set the output width parameter to the video width
    height = this->height; // Set the output height parameter to the video height
    
    // Allocate output frame buffer and copy data from mapped buffer
    frame_buffer.resize(map.size); // Resize the frame buffer to hold the frame data
    
    memcpy(frame_buffer.data(), map.data, map.size); // Copy the frame data from the mapped buffer to the frame buffer. This is necessary because the mapped buffer data will be unmapped and freed after this function returns, so we need to copy it to our own buffer that will persist until the frame is processed

    frame = frame_buffer.data(); // Set the output frame pointer to the frame buffer data. No copy is needed since we will directly use the mapped buffer data
    
    gst_buffer_unmap(buffer, &map); // Unmap the buffer to free resources
    gst_sample_unref(sample); // Unref the sample to free resources

    // We need to keep the sample alive until the frame is processed. 
    return true;
}