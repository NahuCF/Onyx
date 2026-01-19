# MMO Shared Library
project(MMOShared)

set(SHARED_SOURCES
    Source/Types/Types.cpp
    Source/Network/Buffer.cpp
    Source/Network/ENetWrapper.cpp
    Source/Items/Items.cpp
    Source/Spells/AbilityData.cpp
    Source/Map/MapRegistry.cpp
)

set(SHARED_HEADERS
    Source/Types/Types.h
    Source/Network/Buffer.h
    Source/Network/ENetWrapper.h
    Source/Packets/Packets.h
    Source/Items/Items.h
    Source/Spells/SpellDefines.h
    Source/Spells/AbilityData.h
    Source/Map/MapRegistry.h
)

# Check for libpqxx - optional for client, required for servers
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(PQXX libpqxx)
endif()

# Add database source only if pqxx is available
if(PQXX_FOUND)
    list(APPEND SHARED_SOURCES Source/Database/Database.cpp)
    list(APPEND SHARED_HEADERS Source/Database/Database.h)
endif()

add_library(MMOShared STATIC ${SHARED_SOURCES} ${SHARED_HEADERS})

target_include_directories(MMOShared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${enet_SOURCE_DIR}/include
)

target_link_libraries(MMOShared PUBLIC enet)

# Windows socket libraries for ENet
if(WIN32)
    target_link_libraries(MMOShared PUBLIC ws2_32 winmm)
endif()

if(PQXX_FOUND)
    target_include_directories(MMOShared PUBLIC ${PQXX_INCLUDE_DIRS})
    target_link_libraries(MMOShared PUBLIC ${PQXX_LIBRARIES})
    target_link_directories(MMOShared PUBLIC ${PQXX_LIBRARY_DIRS})
    target_compile_definitions(MMOShared PUBLIC HAS_DATABASE=1)
else()
    message(STATUS "libpqxx not found - Database support disabled in MMOShared")
endif()

set_target_properties(MMOShared PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    FOLDER "MMO"
)

# Group source files for IDEs
source_group("Types" FILES
    Source/Types/Types.h
    Source/Types/Types.cpp
)

source_group("Network" FILES
    Source/Network/Buffer.h
    Source/Network/Buffer.cpp
    Source/Network/ENetWrapper.h
    Source/Network/ENetWrapper.cpp
)

source_group("Packets" FILES
    Source/Packets/Packets.h
)

if(PQXX_FOUND)
    source_group("Database" FILES
        Source/Database/Database.h
        Source/Database/Database.cpp
    )
endif()

source_group("Items" FILES
    Source/Items/Items.h
    Source/Items/Items.cpp
)

source_group("Spells" FILES
    Source/Spells/SpellDefines.h
    Source/Spells/AbilityData.h
    Source/Spells/AbilityData.cpp
)

source_group("Map" FILES
    Source/Map/MapRegistry.h
    Source/Map/MapRegistry.cpp
)
