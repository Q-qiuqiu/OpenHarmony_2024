cmake_minimum_required(VERSION 3.17)
project(yolov5_infer)
include_directories(/usr/local/Ascend/ascend-toolkit/)

set(CMAKE_CXX_STANDARD 17)

# ������Ŀ¼
add_subdirectory(src)
