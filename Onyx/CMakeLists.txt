# Onyx Engine Library
project(OnyxEngine)

# Collect source files
file(GLOB_RECURSE ONYX_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

# Remove Model files unless ONYX_USE_ASSIMP is enabled
if(NOT ONYX_USE_ASSIMP)
    list(FILTER ONYX_SOURCES EXCLUDE REGEX ".*Model\\.cpp$")
endif()

# Remove platform-specific files if not on that platform
if(NOT WIN32)
    list(FILTER ONYX_SOURCES EXCLUDE REGEX ".*Windows.*")
endif()
if(NOT UNIX OR APPLE)
    list(FILTER ONYX_SOURCES EXCLUDE REGEX ".*Linux.*")
endif()

# Create the library
add_library(Onyx STATIC ${ONYX_SOURCES})

# Set include directories
target_include_directories(Onyx 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# Precompiled header support (optional, disable if causes issues)
target_precompile_headers(Onyx PRIVATE Source/pch.h)

# Link dependencies
target_link_libraries(Onyx
    PUBLIC
        glfw
        GLEW::GLEW
        OpenGL::GL
        glm::glm
        imgui
        stb_image
        Threads::Threads
)

# Add Assimp if enabled
if(ONYX_USE_ASSIMP)
    target_link_libraries(Onyx PUBLIC assimp)
    target_compile_definitions(Onyx PUBLIC ONYX_USE_ASSIMP)
endif()

# Platform-specific settings
if(WIN32)
    target_compile_definitions(Onyx 
        PUBLIC 
            _CRT_SECURE_NO_WARNINGS
            GLEW_STATIC
        PRIVATE
            SE_PLATFORM_WINDOWS  # For compatibility with existing code
    )
    target_link_libraries(Onyx PUBLIC 
        user32
        gdi32
        shell32
    )
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(Onyx 
        PRIVATE
            SE_PLATFORM_LINUX
    )
    # Linux specific libraries if needed
    target_link_libraries(Onyx PUBLIC 
        dl
        pthread
    )
endif()

# STB Image implementation
target_compile_definitions(Onyx PRIVATE STB_IMAGE_IMPLEMENTATION)

# Enable all warnings in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(Onyx PRIVATE /W4 /WX-)
    else()
        target_compile_options(Onyx PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    endif()
endif()

# Set properties
set_target_properties(Onyx PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    FOLDER "Engine"
)

# Group source files for IDEs
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source Files" FILES ${ONYX_SOURCES})

# Installation rules (commented out for now - optional)
# install(TARGETS Onyx
#     EXPORT OnyxTargets
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
# )

# Install header files (commented out for now)
# install(DIRECTORY Source/
#     DESTINATION include/Onyx
#     FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
#     PATTERN "pch.h" EXCLUDE
# )

# Export targets (commented out for now)
# install(EXPORT OnyxTargets
#     FILE OnyxTargets.cmake
#     NAMESPACE Onyx::
#     DESTINATION lib/cmake/Onyx
# )

# Create config file (commented out for now - optional for installation)
# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#     "${CMAKE_SOURCE_DIR}/cmake/OnyxConfig.cmake.in"
#     "${CMAKE_CURRENT_BINARY_DIR}/OnyxConfig.cmake"
#     INSTALL_DESTINATION lib/cmake/Onyx
#     NO_SET_AND_CHECK_MACRO
#     NO_CHECK_REQUIRED_COMPONENTS_MACRO
# )

# Install config file (commented out for now)
# install(FILES
#     "${CMAKE_CURRENT_BINARY_DIR}/OnyxConfig.cmake"
#     DESTINATION lib/cmake/Onyx
# )