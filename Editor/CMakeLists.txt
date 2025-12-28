# Onyx Editor Application
project(OnyxEditor)

# Collect source files
# Note: Using EditorApp.cpp as the main entry point
# Original Main.cpp references non-existent SE engine and needs migration
set(EDITOR_SOURCES
    EditorApp.cpp
    # Main.cpp        # Disabled - references old SE engine
    # Viewport.cpp    # Disabled - references old SE engine
    # Viewport.h      # Disabled - references old SE engine
)

# Create executable
add_executable(OnyxEditor ${EDITOR_SOURCES})

# Link with Onyx engine
target_link_libraries(OnyxEditor PRIVATE Onyx)

# Include directories
target_include_directories(OnyxEditor 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Onyx
        ${CMAKE_SOURCE_DIR}/Onyx/Source
)

# Platform-specific settings
if(WIN32)
    # Set Windows subsystem for release builds
    set_target_properties(OnyxEditor PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# Set properties
set_target_properties(OnyxEditor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "OnyxEditor"
    FOLDER "Applications"
)

# Copy assets to build directory if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
    add_custom_command(TARGET OnyxEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:OnyxEditor>/Assets"
    )
endif()

# Group source files for IDEs
source_group("Source Files" FILES ${EDITOR_SOURCES})

# Installation rules
install(TARGETS OnyxEditor
    RUNTIME DESTINATION bin
)

# Install assets if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets")
    install(DIRECTORY Assets/
        DESTINATION bin/Assets
    )
endif()