cmake_minimum_required(VERSION 3.4.1)

include_directories(src/main/cpp/core)
include_directories(src/main/cpp/core/android)
include_directories(src/main/cpp/modules)
include_directories(src/main/cpp/modules/demux)

# cmakelists 设置c++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    #elseif(COMPILER_SUPPORTS_CXX0X)
    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
# cmakelists 设置c++11

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_GLEXT_PROTOTYPES")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -DGL_GLEXT_PROTOTYPES=1 -D__ANDROID_BUILD__")

add_definitions(-Wall -DGL_GLEXT_PROTOTYPES=1 -D__ANDROID_BUILD__)

add_library(playercorejni

             SHARED

        src/main/cpp/core/memory_manager.c
        src/main/cpp/core/audio_out.c
        src/main/cpp/core/video_out.c
        src/main/cpp/core/playercore.c
        src/main/cpp/core/android/os.cpp
        src/main/cpp/core/android/droid_audio_openSL.cpp
#        src/main/cpp/core/android/droid_audio.cpp
        src/main/cpp/core/android/droid_video.cpp
        src/main/cpp/core/android/RenderingEnine1.cpp
        src/main/cpp/core/android/RenderingEnine2.cpp
        src/main/cpp/core/android/mc.cpp
        src/main/cpp/core/android/cpu-features.c
        src/main/cpp/modules/decoder/FFMPEG/ffmpegDecoder.c
        src/main/cpp/modules/demux/demux_FFMPEG/demux_FFMPEG.c
        src/main/cpp/modules/demux/demux_rtmp/fa_flv2es.c
        src/main/cpp/modules/demux/demux_rtmp/rtmp_demux.cpp
        src/main/cpp/modules/demux/demux_srs_rtmp/srs_librtmp.cpp
        src/main/cpp/modules/demux/demux_srs_rtmp/srs_rtmp_demux.cpp
        src/main/cpp/modules/decoder/AAC/fdkaacdec.c
        )

add_library( FraunhoferAAC SHARED IMPORTED)
set_target_properties( FraunhoferAAC
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libFraunhoferAAC.so)

add_library( rtmp SHARED IMPORTED)
set_target_properties( rtmp
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/librtmp.so)

add_library( FFMPEG SHARED IMPORTED)
set_target_properties( FFMPEG
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libFFMPEG.so)

find_library( log-lib
              log )

target_link_libraries(  playercorejni

                        android
                        GLESv1_CM    #OpenGL ES 1.0 库
                        GLESv2       #OpenGL ES 2.0 库
                        EGL          #EGL环境相关库
                        OpenSLES

                        FraunhoferAAC
                        rtmp
                        FFMPEG


                       log )