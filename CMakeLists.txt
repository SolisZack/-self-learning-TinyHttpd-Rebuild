cmake_minimum_required(VERSION 3.17)
project(MyHttpd)

INCLUDE_DIRECTORIES(include)
AUX_SOURCE_DIRECTORY(src SRC_DIR)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D CHECK -std=c++11 -latomic -pthread")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(MyHttpd ${SRC_DIR})