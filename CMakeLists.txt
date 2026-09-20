cmake_minimum_required(VERSION 3.20)
project(blackhole2_0)

set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# Include GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/glfw)

# GLAD
add_library(glad src/glad/src/glad.c)
target_include_directories(glad PUBLIC src/glad/include)

# Include ImGui
set(imgui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui)
file(GLOB IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui_lib ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC glfw glad)

# Main executable
add_executable(blackhole2.0 src/main.cpp src/Application.cpp src/Renderer.cpp src/Shader.cpp src/UIManager.cpp src/Cinematic.cpp resources.rc)
target_link_libraries(blackhole2.0 PRIVATE glfw glad imgui_lib)

if(MSVC)
    set_target_properties(blackhole2.0 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()
