cmake_minimum_required(VERSION 3.16.3)
project(rtspcam)

set(CMAKE_CXX_STANDARD 20)

if (DEBUG)
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wno-deprecated-declarations")
    set(CMAKE_BUILD_RPATH)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    add_definitions(-DNDEBUG)
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

find_package(PkgConfig)
find_package(Threads)
find_package(fmt)
find_package(Boost)

pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_RTSP REQUIRED gstreamer-rtsp-1.0)
pkg_check_modules(GST_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0)
pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)
pkg_check_modules(ARAVIS REQUIRED aravis-0.10)

include_directories(
        ${GLIB_INCLUDE_DIRS}
        ${GST_INCLUDE_DIRS}
        ${GST_APP_INCLUDE_DIRS}
        ${GST_VIDEO_INCLUDE_DIRS}
        ${GST_RTSP_INCLUDE_DIRS}
        ${GST_RTSP_SERVER_INCLUDE_DIRS}
        ${ARAVIS_INCLUDE_DIRS}
        ${GDK_PIXBUF_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GST_LIBRARY_DIRS}
        ${GST_APP_LIBRARY_DIRS}
        ${GST_VIDEO_LIBRARY_DIRS}
        ${GST_RTSP_LIBRARY_DIRS}
        ${GST_RTSP_LIBRARY_DIRS}
        ${ARAVIS_LIBRARY_DIRS}
        ${GDK_PIXBUF_LIBRARY_DIRS}
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src sources)

add_executable(${PROJECT_NAME} main.cpp ${sources})
target_link_libraries(${PROJECT_NAME} PUBLIC
        fmt::fmt
        pthread
        ${GLIB_LIBRARIES}
        ${GST_LIBRARIES}
        ${GST_APP_LIBRARIES}
        ${GST_VIDEO_LIBRARIES}
        ${GST_RTSP_LIBRARIES}
        ${GST_RTSP_SERVER_LIBRARIES}
        ${ARAVIS_LIBRARIES}
        ${GDK_PIXBUF_LIBRARIES}
)