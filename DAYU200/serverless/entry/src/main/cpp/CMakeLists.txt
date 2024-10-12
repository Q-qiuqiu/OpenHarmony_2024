# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)
project(OpencvSample)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(OPENCV_LIB_PATH ${NATIVERENDER_ROOT_PATH}/../../../libs/${OHOS_ARCH})

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

add_library(entry SHARED
    common/common.cpp
    hello.cpp)

target_link_libraries(entry PUBLIC
    libace_napi.z.so
    librawfile.z.so
    libhilog_ndk.z.so
    ${OPENCV_LIB_PATH}/libopencv_core.so
    opencv_imgcodecs
    opencv_imgproc
    opencv_photo
    opencv_videoio
    opencv_flann
    opencv_features2d
    opencv_calib3d
    opencv_video
    opencv_highgui
    opencv_ml
    opencv_stitching
    opencv_dnn
    opencv_objdetect
    opencv_gapi)