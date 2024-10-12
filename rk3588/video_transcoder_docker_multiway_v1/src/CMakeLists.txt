cmake_minimum_required(VERSION 3.17)
project(video_transcoder)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libswscale
        libavutil
)
find_package(OpenCV REQUIRED)

add_executable(video_transcoder main.cpp video_transcoder.cpp)

target_link_libraries(video_transcoder PRIVATE PkgConfig::LIBAV ${OpenCV_LIBS})

install(TARGETS video_transcoder DESTINATION bin)
