# MMO Login Server
project(MMOLoginServer)

set(LOGINSERVER_SOURCES
    Source/Main.cpp
    Source/LoginServer.cpp
)

set(LOGINSERVER_HEADERS
    Source/LoginServer.h
)

add_executable(MMOLoginServer ${LOGINSERVER_SOURCES} ${LOGINSERVER_HEADERS})

target_include_directories(MMOLoginServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(MMOLoginServer PRIVATE
    MMOShared
    OpenSSL::Crypto
)

set_target_properties(MMOLoginServer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MMOLoginServer"
    FOLDER "MMO"
)

# Group source files for IDEs
source_group("Source Files" FILES ${LOGINSERVER_SOURCES})
source_group("Header Files" FILES ${LOGINSERVER_HEADERS})
