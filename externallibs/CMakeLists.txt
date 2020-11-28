cmake_minimum_required (VERSION 3.8)

add_library(SDL2)

target_include_directories(SDL2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")