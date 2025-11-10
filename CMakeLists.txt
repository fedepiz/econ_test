cmake_minimum_required(VERSION 3.5)
project(EconTest)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Main)

set_property(TARGET Main PROPERTY CXX_STANDARD 20)

target_sources(Main PRIVATE src/main.cpp)
target_include_directories(Main PRIVATE include)
