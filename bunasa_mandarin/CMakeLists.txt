# CMake project for bunasa_mandarin
cmake_minimum_required(VERSION 3.12)

# Set the project name and version
project(bunasa_mandarin VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files for the project
add_executable(bunasa_mandarin 
    "bunasa_mandarin.cpp" 
    "Headers/bunasa_mandarin.h" 
    "info.cpp" 
    "Headers/info.h" 
    "updater.cpp"
    # ImGui core files
    "thirdparty/imgui/imgui.cpp"
    "thirdparty/imgui/imgui_draw.cpp"
    "thirdparty/imgui/imgui_widgets.cpp"
    "thirdparty/imgui/imgui_tables.cpp"
    # ImGui backends for GLFW and OpenGL3
    "thirdparty/imgui/backends/imgui_impl_glfw.cpp"
    "thirdparty/imgui/backends/imgui_impl_opengl3.cpp"
)

# Include directories for ImGui
target_include_directories(bunasa_mandarin PRIVATE 
    "thirdparty/imgui"
    "thirdparty/imgui/backends"
)

# Link necessary libraries (GLFW, OpenGL, and dl for Linux)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(bunasa_mandarin 
    glfw 
    OpenGL::GL 
    dl
)
