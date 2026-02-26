cmake_minimum_required(VERSION 3.16)
project(GameModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Provided by editor Build Project button.
set(VELIX_PREFIX "${CMAKE_PREFIX_PATH}")

file(GLOB_RECURSE PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/*.cpp")

add_library(GameModule SHARED ${PROJECT_SOURCES})

target_include_directories(GameModule PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Sources"
    "${VELIX_PREFIX}/../Engine/include"
    "${VELIX_PREFIX}/../Core/include"
    "${VELIX_PREFIX}/../VelixSDK/include"
    "${VELIX_PREFIX}/../external/glm"
)
