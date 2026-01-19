# MMO Client
project(MMOClient)

set(CLIENT_SOURCES
    Source/Main.cpp
    Source/GameClient.cpp
)

set(CLIENT_HEADERS
    Source/GameClient.h
)

add_executable(MMOClient ${CLIENT_SOURCES} ${CLIENT_HEADERS})

target_include_directories(MMOClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Onyx
    ${CMAKE_SOURCE_DIR}/Onyx/Source
)

target_link_libraries(MMOClient PRIVATE
    MMOShared
    Onyx
)

set_target_properties(MMOClient PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MMOClient"
    FOLDER "MMO"
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(MMOClient PRIVATE ws2_32 winmm)
    set_target_properties(MMOClient PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# Group source files for IDEs
source_group("Source Files" FILES ${CLIENT_SOURCES})
source_group("Header Files" FILES ${CLIENT_HEADERS})
