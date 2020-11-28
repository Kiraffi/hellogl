cmake_minimum_required (VERSION 3.8)

add_library(MyGlad
	glad/glad.h
	glad/glad.c
	glad/khrplatform.h
	)

target_include_directories(MyLibraries PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")