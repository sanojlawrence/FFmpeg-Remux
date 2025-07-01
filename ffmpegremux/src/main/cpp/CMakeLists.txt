cmake_minimum_required(VERSION 3.10.2)
project("ffmpegwrapper")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-D__STDC_LIMIT_MACROS)

# Include headers
include_directories(${CMAKE_SOURCE_DIR}/../../../../generated/${ANDROID_ABI}/include)

# Native source files
add_library(ffmpegwrapper SHARED
        ffmpeg_loader.cpp
        ffmpeg_main.cpp
)

# Prebuilt FFmpeg shared libraries
set(FFMPEG_LIBS_DIR ${CMAKE_SOURCE_DIR}/../../../../generated/${ANDROID_ABI}/lib/)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIBS_DIR}/libavcodec.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIBS_DIR}/libavformat.so)

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIBS_DIR}/libavutil.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FFMPEG_LIBS_DIR}/libswresample.so)

# Native Android libraries
find_library(log-lib log)
find_library(android-lib android)

# Link all
target_link_libraries(ffmpegwrapper
        avformat
        avcodec
        avutil
        swresample
        ${log-lib}
        ${android-lib}
)
