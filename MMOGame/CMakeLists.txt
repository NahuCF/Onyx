# MMO Game Project
project(MMOGame)

# Set source files
set(MMOGAME_SOURCES
    Main.cpp
)

# Create executable
add_executable(MMOGame ${MMOGAME_SOURCES})

# Link with Onyx engine
target_link_libraries(MMOGame PRIVATE Onyx)

# Include directories
target_include_directories(MMOGame 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Onyx
        ${CMAKE_SOURCE_DIR}/Onyx/Source
)

# Platform-specific settings
if(WIN32)
    # Windows subsystem for release builds (no console)
    set_target_properties(MMOGame PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# Set properties
set_target_properties(MMOGame PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MMOGame"
    FOLDER "Games"
)

# Copy assets to build directory if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
    add_custom_command(TARGET MMOGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:MMOGame>/Assets"
    )
endif()

# Group source files for IDEs
source_group("Source Files" FILES ${MMOGAME_SOURCES})

# Installation rules
install(TARGETS MMOGame
    RUNTIME DESTINATION bin
)

# Install assets if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
    install(DIRECTORY Assets/
        DESTINATION bin/Assets
    )
endif()