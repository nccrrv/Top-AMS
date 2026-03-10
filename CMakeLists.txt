# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)



set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# 关闭所有 C++ 警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
# 关闭所有 C 警告（如果项目中包含 C 文件）
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Top-AMS)

