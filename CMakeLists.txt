cmake_minimum_required(VERSION 3.10)

set(proj mobjtex)
project(${proj})
set(CMAKE_CXX_STANDARD 20) 

add_executable(${proj} main.cpp)

add_subdirectory(fmt-8.1.1)

target_include_directories(${proj} PRIVATE D:/doc/libs/glm)

target_link_libraries(${proj} PRIVATE fmt)