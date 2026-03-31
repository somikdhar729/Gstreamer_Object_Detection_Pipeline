cmake_minimum_required(VERSION 3.25)
project(Gstreamer_Object_Detection_Pipeline)

set(CMAKE_CXX_STANDARD 20)

# GStreamer
set(GSTREAMER_ROOT_DIR "C:/Program Files/gstreamer/1.0/msvc_x86_64")
include_directories(
    ${GSTREAMER_ROOT_DIR}/include
    ${GSTREAMER_ROOT_DIR}/include/gstreamer-1.0
    ${GSTREAMER_ROOT_DIR}/include/glib-2.0
    ${GSTREAMER_ROOT_DIR}/lib/glib-2.0/include
)
link_directories(
    ${GSTREAMER_ROOT_DIR}/lib
)

# Executable for reading and rendering video using Gstreamer
add_executable(app src/Video_Read_and_Render.cpp)

# Link the GStreamer libraries to the executable
target_link_libraries(app
    gstreamer-1.0
    gstapp-1.0
    gstbase-1.0
    gobject-2.0
    glib-2.0
)