# CMakeList.txt : CMake project for hellogl, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (font_render "src/main_font_renderer.cpp")

target_link_libraries(font_render PRIVATE MyGlad)
target_link_libraries(font_render PRIVATE MyLibraries)
target_link_libraries(font_render PRIVATE ${SDL2_LIBRARY})
# TODO: Add tests and install targets if needed.
