# MMO Shared Library
project(MMOShared)

set(SHARED_SOURCES
    Source/Types/Types.cpp
    Source/Network/Buffer.cpp
    Source/Network/ENetWrapper.cpp
    Source/Items/Items.cpp
    Source/Spells/AbilityData.cpp
    Source/Map/MapRegistry.cpp
    Source/Terrain/ChunkFileReader.cpp
    Source/Terrain/ChunkFileWriter.cpp
    Source/Terrain/TerrainMeshGenerator.cpp
    Source/Model/OmdlWriter.cpp
    Source/Model/OmdlReader.cpp
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
    Source/Terrain/ChunkFormat.h
    Source/Terrain/ChunkIO.h
    Source/Terrain/TerrainData.h
    Source/Terrain/ChunkFileReader.h
    Source/Terrain/ChunkFileWriter.h
    Source/Terrain/TerrainMeshGenerator.h
    Source/Model/OmdlFormat.h
    Source/Model/OmdlWriter.h
    Source/Model/OmdlReader.h
)

# Check for libpqxx - optional for client, required for servers.
# On Windows, vcpkg manifest mode provides a CMake config-mode package.
# On Linux, fall back to pkg-config (libpqxx-dev).
find_package(libpqxx CONFIG QUIET)
if(libpqxx_FOUND)
    set(PQXX_FOUND TRUE)
    set(PQXX_TARGET libpqxx::pqxx)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PQXX libpqxx)
    endif()
endif()

# Add database source only if pqxx is available
if(PQXX_FOUND)
    list(APPEND SHARED_SOURCES Source/Database/Database.cpp)
    list(APPEND SHARED_HEADERS Source/Database/Database.h)
    list(APPEND SHARED_SOURCES Source/Database/MigrationRunner.cpp)
    list(APPEND SHARED_HEADERS Source/Database/MigrationRunner.h)
    list(APPEND SHARED_SOURCES Source/Data/GameDataStore.cpp)
    list(APPEND SHARED_HEADERS Source/Data/GameDataStore.h)
endif()

add_library(MMOShared STATIC ${SHARED_SOURCES} ${SHARED_HEADERS})

target_include_directories(MMOShared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${enet_SOURCE_DIR}/include
)

target_link_libraries(MMOShared PUBLIC enet glm::glm)

# Windows socket libraries for ENet
if(WIN32)
    target_link_libraries(MMOShared PUBLIC ws2_32 winmm)
endif()

if(PQXX_FOUND)
    if(PQXX_TARGET)
        target_link_libraries(MMOShared PUBLIC ${PQXX_TARGET})
    else()
        target_include_directories(MMOShared PUBLIC ${PQXX_INCLUDE_DIRS})
        target_link_libraries(MMOShared PUBLIC ${PQXX_LIBRARIES})
        target_link_directories(MMOShared PUBLIC ${PQXX_LIBRARY_DIRS})
    endif()
    target_compile_definitions(MMOShared PUBLIC HAS_DATABASE=1)
else()
    message(STATUS "libpqxx not found - Database support disabled in MMOShared")
endif()

set_target_properties(MMOShared PROPERTIES
    CXX_STANDARD 20
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
        Source/Database/MigrationRunner.h
        Source/Database/MigrationRunner.cpp
    )
    source_group("Data" FILES
        Source/Data/GameDataStore.h
        Source/Data/GameDataStore.cpp
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

source_group("Terrain" FILES
    Source/Terrain/ChunkFormat.h
    Source/Terrain/ChunkIO.h
    Source/Terrain/TerrainData.h
    Source/Terrain/ChunkFileReader.h
    Source/Terrain/ChunkFileReader.cpp
    Source/Terrain/ChunkFileWriter.h
    Source/Terrain/ChunkFileWriter.cpp
    Source/Terrain/TerrainMeshGenerator.h
    Source/Terrain/TerrainMeshGenerator.cpp
)

source_group("Model" FILES
    Source/Model/OmdlFormat.h
    Source/Model/OmdlWriter.h
    Source/Model/OmdlWriter.cpp
    Source/Model/OmdlReader.h
    Source/Model/OmdlReader.cpp
)
