# MMO World Server
project(MMOWorldServer)

set(WORLDSERVER_SOURCES
    Source/Main.cpp
    Source/WorldServer.cpp
    Source/Entity/Entity.cpp
    Source/Map/MapDefines.cpp
    Source/Map/MapInstance.cpp
    Source/Map/MapManager.cpp
    Source/Map/MapTemplates.cpp
    Source/Grid/Grid.cpp
    Source/AI/ScriptedAI.cpp
    Source/AI/ScriptRegistry.cpp
    Source/AI/CreatureTemplates.cpp
)

set(WORLDSERVER_HEADERS
    Source/WorldServer.h
    # Entity
    Source/Entity/Entity.h
    Source/Entity/Components.h
    Source/Entity/AuraComponent.h
    # AI
    Source/AI/EventMap.h
    Source/AI/AIDefines.h
    Source/AI/AICallbacks.h
    Source/AI/CreatureTemplate.h
    Source/AI/CreatureTemplates.h
    Source/AI/ConditionEvaluator.h
    Source/AI/DataDrivenAI.h
    Source/AI/ScriptedAI.h
    Source/AI/ScriptRegistry.h
    Source/AI/SummonList.h
    # Scripts
    Source/Scripts/ScriptLoader.h
    Source/Scripts/ShadowLordAI.h
    # Map
    Source/Map/MapDefines.h
    Source/Map/MapInstance.h
    Source/Map/MapManager.h
    Source/Map/MapTemplates.h
    # Grid
    Source/Grid/GridDefines.h
    Source/Grid/GridCell.h
    Source/Grid/Grid.h
)

add_executable(MMOWorldServer ${WORLDSERVER_SOURCES} ${WORLDSERVER_HEADERS})

target_include_directories(MMOWorldServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(MMOWorldServer PRIVATE
    MMOShared
)

set_target_properties(MMOWorldServer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MMOWorldServer"
    FOLDER "MMO"
)

# Group source files for IDEs
source_group("Source Files" FILES
    Source/Main.cpp
    Source/WorldServer.cpp
)

source_group("Entity" FILES
    Source/Entity/Entity.h
    Source/Entity/Components.h
    Source/Entity/AuraComponent.h
    Source/Entity/Entity.cpp
)

source_group("AI" FILES
    Source/AI/EventMap.h
    Source/AI/AIDefines.h
    Source/AI/AICallbacks.h
    Source/AI/CreatureTemplate.h
    Source/AI/CreatureTemplates.h
    Source/AI/CreatureTemplates.cpp
    Source/AI/ConditionEvaluator.h
    Source/AI/DataDrivenAI.h
    Source/AI/ScriptedAI.h
    Source/AI/ScriptedAI.cpp
    Source/AI/ScriptRegistry.h
    Source/AI/ScriptRegistry.cpp
    Source/AI/SummonList.h
)

source_group("Scripts" FILES
    Source/Scripts/ScriptLoader.h
    Source/Scripts/ShadowLordAI.h
)

source_group("Map" FILES
    Source/Map/MapDefines.h
    Source/Map/MapDefines.cpp
    Source/Map/MapInstance.h
    Source/Map/MapInstance.cpp
    Source/Map/MapManager.h
    Source/Map/MapManager.cpp
    Source/Map/MapTemplates.h
    Source/Map/MapTemplates.cpp
)

source_group("Grid" FILES
    Source/Grid/GridDefines.h
    Source/Grid/GridCell.h
    Source/Grid/Grid.h
    Source/Grid/Grid.cpp
)
