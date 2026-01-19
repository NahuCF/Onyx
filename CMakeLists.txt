cmake_minimum_required(VERSION 3.21...4.2)
project(Onyx VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Export compile commands for VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform detection
if(WIN32)
    set(ONYX_PLATFORM_WINDOWS TRUE)
    add_compile_definitions(ONYX_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(ONYX_PLATFORM_LINUX TRUE)
    add_compile_definitions(ONYX_PLATFORM_LINUX)
elseif(APPLE)
    set(ONYX_PLATFORM_MACOS TRUE)
    add_compile_definitions(ONYX_PLATFORM_MACOS)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enable folder structure in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Options
option(ONYX_BUILD_EDITOR "Build the Onyx Editor" ON)
option(ONYX_BUILD_MMOGAME "Build the MMO Game" ON)
option(ONYX_BUILD_EXAMPLES "Build example projects" OFF)
option(ONYX_USE_SYSTEM_LIBS "Use system installed libraries instead of fetching" OFF)

# Include FetchContent for dependency management
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Allow older CMake compatibility in fetched dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# ========== External Dependencies ==========

# GLFW
if(NOT ONYX_USE_SYSTEM_LIBS)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.9
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
else()
    find_package(glfw3 REQUIRED)
endif()

# GLEW (for OpenGL extension loading)
if(NOT ONYX_USE_SYSTEM_LIBS)
    FetchContent_Declare(
        glew-cmake
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
        GIT_TAG        glew-cmake-2.2.0
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glew-cmake)
    add_library(GLEW::GLEW ALIAS libglew_static)
else()
    find_package(GLEW REQUIRED)
endif()

# GLM (math library)
if(NOT ONYX_USE_SYSTEM_LIBS)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0.9.9.8
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(glm)
else()
    find_package(glm REQUIRED)
endif()

# Dear ImGui (docking branch for dockspace and multi-viewport support)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(imgui)

# Create ImGui library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLEW)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")

# stb_image (header-only image loading)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(stb)

# Create stb_image interface library
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})

# Assimp (3D model loading) - Required for 3D Editor
option(ONYX_USE_ASSIMP "Include Assimp for 3D model loading" ON)
if(ONYX_USE_ASSIMP)
    if(NOT ONYX_USE_SYSTEM_LIBS)
        FetchContent_Declare(
            assimp
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG        v5.3.1
            GIT_SHALLOW    TRUE
            GIT_PROGRESS   TRUE
        )
        set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
        set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(assimp)
    else()
        find_package(assimp REQUIRED)
    endif()
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find Threads (required for some platforms)
find_package(Threads REQUIRED)

# ========== Add subdirectories ==========
add_subdirectory(Onyx)

if(ONYX_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

if(ONYX_BUILD_MMOGAME)
    add_subdirectory(MMOGame)
endif()

# ========== Installation rules ==========
include(GNUInstallDirs)
install(TARGETS Onyx
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)