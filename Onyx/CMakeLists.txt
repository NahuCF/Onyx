cmake_minimum_required(VERSION 3.10)

# Project name
project(Onyx VERSION 1.0)

# Include files to copile
file(GLOB_RECURSE SOURCES "*.cpp")
file(GLOB_RECURSE EXCLUDE_SOURCES "Source/Vendor/*")
list(REMOVE_ITEM SOURCES ${EXCLUDE_SOURCES})

# Set output
add_library(Onyx STATIC ${SOURCES})

# Ad include paths
include_directories(${CMAKE_SOURCE_DIR})
target_include_directories(Onyx PRIVATE ${CMAKE_SOURCE_DIR}/Source/Maths)
target_include_directories(Onyx PRIVATE ${CMAKE_SOURCE_DIR}/Source/Vendor/assimp/include)

# Add definitions
target_compile_definitions(Onyx PRIVATE STB_IMAGE_IMPLEMENTATION)




