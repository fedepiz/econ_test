cmake_minimum_required(VERSION 3.10)
project(EconTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Main)

set_property(TARGET Main PROPERTY CXX_STANDARD 20)

target_sources(Main PRIVATE src/main.cpp src/simulation.cpp src/core.cpp)
#Imgui
target_sources(Main PRIVATE deps/imgui/imgui.cpp deps/imgui/imgui_tables.cpp deps/imgui/imgui_widgets.cpp deps/imgui/imgui_draw.cpp)
#Imgui/Raylib
target_sources(Main PRIVATE deps/rlImGui/rlImGui.cpp)

target_include_directories(Main PRIVATE include)

target_include_directories(Main PRIVATE deps/include/raylib)
target_include_directories(Main PRIVATE deps/imgui)
target_include_directories(Main PRIVATE deps/rlImGui)

add_subdirectory(deps/raylib)
target_link_libraries(Main PUBLIC raylib)

if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreVideo -framework Cocoa -framework IOKit")
  target_link_directories(Main PRIVATE deps/libs/arm_64)
  target_link_libraries(Main PRIVATE glfw3)
endif()
