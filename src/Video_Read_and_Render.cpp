#include <gst/gst.h>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <conio.h> // For _kbhit() and _getch()
#endif

int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "Wrong usage! Correct usage: " << argv[0] << " <video_file_path>" << std::endl;
        return -1;
    }

    // Initialize GStreamer
    gst_init(&argc, &argv);
    /*
    Must be first GStreamer command. It initializes the GStreamer library and should be called before any other GStreamer functions. It takes the command-line arguments to allow GStreamer to process any relevant options.
    */

    // If windows, fix the path (replace backslashes with forward slashes)
    std::string path = argv[1];
    #ifdef _WIN32
        std::replace(path.begin(), path.end(), '\\', '/');
    #endif

    // Build the GStreamer pipeline string
    std::stringstream pipeline_str;
    pipeline_str << "filesrc location=\"" << path << "\" ! "
                 << "qtdemux ! h264parse ! nvh264dec ! "
                //  << "autovideosink";
                 << "d3d11videosink";
    // Explanation of pipeline:
    // - filesrc: Reads the video file from disk.
    // - qtdemux: Demultiplexes the MP4 container to extract the H.264 video stream.
    // - h264parse: Parses the H.264 stream to ensure it's in the correct format for decoding.
    // - nvh264dec: Uses NVIDIA's hardware-accelerated decoder to decode the video frames.
    // - autovideosink: Automatically selects an appropriate video sink to display the video.
    std::cout << "Pipeline: " << pipeline_str.str() << std::endl;

    // Create the GStreamer pipeline
    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipeline_str.str().c_str(), &error);
    if (!pipeline) {
        std::cerr << "Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        return -1;
    }

    // Start playing the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS(End of Stream)
    GstBus *bus = gst_element_get_bus(pipeline); // Get the bus to listen for messages from the pipeline
    // What is GstBus? A GstBus is a messaging system used in GStreamer to communicate between different elements of the pipeline. It allows elements to post messages about their state, errors, or other events, which can be listened to and handled by the application.

    bool running = true;
    bool paused = false;
    
    while(running){
        // Check whether the esc key is pressed to exit the loop
        #ifdef _WIN32
            if(_kbhit()){
                char c = _getch();
                if(c == 27){
                    std::cout << "ESC pressed. Exiting..." << std::endl;
                    running = false;
                    break;
                }
                if(c == ' '){
                    paused = !paused;
                    gst_element_set_state(pipeline, paused ? GST_STATE_PAUSED : GST_STATE_PLAYING);
                }
            }
        #endif

        // Wait for messages on the bus with a timeout (non-blocking)
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)); // Waits for either an error message or an end-of-stream message, with a timeout of 100 milliseconds to keep the loop responsive. This allows the application to check for user input (like ESC key) while still processing GStreamer messages.
        
        // Handle the received message
        if(msg != nullptr){
            switch(GST_MESSAGE_TYPE(msg)){
                case GST_MESSAGE_ERROR:
                    GError* err;
                    gchar* debug_info;
                    gst_message_parse_error(msg, &err, &debug_info);
                    std::cerr << "Error received from element " << err->message << std::endl;
                    g_error_free(err);
                    g_free(debug_info);
                    running = false;
                    break;
                case GST_MESSAGE_EOS:
                    std::cout << "End-Of-Stream reached." << std::endl;
                    running = false;
                    break;
                default:
                    // We should not reach here because we only asked for ERRORs and EOS
                    std::cerr << "Unexpected message received." << std::endl;
                    break;                
            }
            gst_message_unref(msg); // Free the message after processing
        }       
    }

    // Free the resources
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL); // Stop the pipeline
    gst_object_unref(pipeline); // Free the pipeline object
    return 0;
}