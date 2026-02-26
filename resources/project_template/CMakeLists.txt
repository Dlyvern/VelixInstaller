cmake_minimum_required(VERSION 3.16)
project(GameModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(VelixSDK CONFIG REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/*.cpp")

add_library(GameModule SHARED ${PROJECT_SOURCES})

target_include_directories(GameModule PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Sources")
target_link_libraries(GameModule PRIVATE VelixSDK::VelixSDK)
