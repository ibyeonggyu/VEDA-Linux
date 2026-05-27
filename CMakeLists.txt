cmake_minimum_required(VERSION 3.10)

project(Project)

set(CMAKE_C_STANDARD 11)

include_directories(${PROJECT_SOURCE_DIR}/devices)

add_subdirectory(devices)
add_subdirectory(server)
