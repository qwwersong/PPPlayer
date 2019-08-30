cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

include_directories(src/main/cpp/ffmpeg/include)
include_directories(src/main/cpp/soundtouch/include)
include_directories(src/main/cpp/soundtouch/SoundTouch)

# cmakelists 设置c++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")

endif()
# cmakelists 设置c++11

add_library(
             Player

             SHARED

             src/main/cpp/Player.cpp
             src/main/cpp/ffmpeg/FFmpegManager.cpp
             src/main/cpp/ffmpeg/PlayStatus.cpp
             src/main/cpp/ffmpeg/SLAudio.cpp
             src/main/cpp/ffmpeg/SLQueue.cpp
             src/main/cpp/ffmpeg/SLVideo.cpp
             src/main/cpp/ffmpeg/CallJava.cpp
             src/main/cpp/gles/BaseSurface.cpp
             src/main/cpp/gles/EGLHelper.cpp
             src/main/cpp/gles/EGLThread.cpp

             src/main/cpp/soundtouch/SoundTouch/AAFilter.cpp
             src/main/cpp/soundtouch/SoundTouch/FIFOSampleBuffer.cpp
             src/main/cpp/soundtouch/SoundTouch/FIRFilter.cpp
             src/main/cpp/soundtouch/SoundTouch/cpu_detect_x86.cpp
             src/main/cpp/soundtouch/SoundTouch/sse_optimized.cpp
             src/main/cpp/soundtouch/SoundTouch/RateTransposer.cpp
             src/main/cpp/soundtouch/SoundTouch/SoundTouch.cpp
             src/main/cpp/soundtouch/SoundTouch/InterpolateCubic.cpp
             src/main/cpp/soundtouch/SoundTouch/InterpolateLinear.cpp
             src/main/cpp/soundtouch/SoundTouch/InterpolateShannon.cpp
             src/main/cpp/soundtouch/SoundTouch/TDStretch.cpp
             src/main/cpp/soundtouch/SoundTouch/PeakFinder.cpp)

add_library( avcodec-57 SHARED IMPORTED)
set_target_properties( avcodec-57
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavcodec-57.so)


add_library( avdevice-57 SHARED IMPORTED)
set_target_properties( avdevice-57
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavdevice-57.so)


add_library( avfilter-6 SHARED IMPORTED)
set_target_properties( avfilter-6
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavfilter-6.so)


add_library( avformat-57 SHARED IMPORTED)
set_target_properties( avformat-57
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavformat-57.so)


add_library( avutil-55 SHARED IMPORTED)
set_target_properties( avutil-55
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavutil-55.so)

add_library( postproc-54 SHARED IMPORTED)
set_target_properties( postproc-54
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libpostproc-54.so)

add_library( swresample-2 SHARED IMPORTED)
set_target_properties( swresample-2
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libswresample-2.so)


add_library( swscale-4 SHARED IMPORTED)
set_target_properties( swscale-4
                       PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libswscale-4.so)


find_library( log-lib
              log )


target_link_libraries(
                       Player

                       android      #ANativeWindow相关库
                       GLESv2       #OpenGL ES 2.0 库
                       EGL          #EGL环境相关库
                       OpenSLES

                       avcodec-57
                       avdevice-57
                       avfilter-6
                       avformat-57
                       avutil-55
                       postproc-54
                       swresample-2
                       swscale-4

                        log )