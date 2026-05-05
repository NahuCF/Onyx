# MMO World Server
project(MMOWorldServer)

set(WORLDSERVER_SOURCES
    Source/Main.cpp
    Source/WorldServer.cpp
    Source/Entity/Entity.cpp
    Source/Map/MapDefines.cpp
    Source/Map/MapInstance.cpp
    Source/Map/MapManager.cpp
    Source/Grid/Grid.cpp
    Source/AI/CreatureAI.cpp
    Source/AI/CreatureTemplates.cpp
    Source/AI/CreatureScripts.cpp
    Source/AI/InstanceScripts.cpp
    Source/Triggers/TriggerScripts.cpp
    Source/Scripting/GameObjectScripts.cpp
    Source/Scripting/QuestScripts.cpp
    Source/Scripting/SpellScripts.cpp
    Source/Scripting/PlayerScripts.cpp
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
    Source/AI/CreatureTemplate.h
    Source/AI/CreatureTemplates.h
    Source/AI/ConditionEvaluator.h
    Source/AI/DataDrivenAI.h
    Source/AI/CreatureAI.h
    Source/AI/CreatureScript.h
    Source/AI/BuiltInAI.h
    Source/AI/InstanceScript.h
    Source/AI/SummonList.h
    # Scripts
    Source/Scripts/ShadowLordAI.h
    # Scripting interfaces
    Source/Scripting/IEntity.h
    Source/Scripting/IMapContext.h
    Source/Scripting/GameObjectScript.h
    Source/Scripting/QuestScript.h
    Source/Scripting/SpellScript.h
    Source/Scripting/PlayerScript.h
    # Map
    Source/Map/MapDefines.h
    Source/Map/MapInstance.h
    Source/Map/MapManager.h
    # Grid
    Source/Grid/GridDefines.h
    Source/Grid/GridCell.h
    Source/Grid/Grid.h
    # Triggers
    Source/Triggers/TriggerScript.h
)

add_executable(MMOWorldServer ${WORLDSERVER_SOURCES} ${WORLDSERVER_HEADERS})

target_include_directories(MMOWorldServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# Avoid Windows.h's min/max macros colliding with std::min/std::max
# (AuraComponent.h uses std::max). Onyx sets these PUBLIC; WorldServer
# doesn't link Onyx, so we need them locally.
if(WIN32)
    target_compile_definitions(MMOWorldServer PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

target_link_libraries(MMOWorldServer PRIVATE
    MMOShared
)

set_target_properties(MMOWorldServer PROPERTIES
    CXX_STANDARD 20
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
    Source/AI/CreatureTemplate.h
    Source/AI/CreatureTemplates.h
    Source/AI/CreatureTemplates.cpp
    Source/AI/ConditionEvaluator.h
    Source/AI/DataDrivenAI.h
    Source/AI/CreatureAI.h
    Source/AI/CreatureAI.cpp
    Source/AI/CreatureScript.h
    Source/AI/CreatureScripts.cpp
    Source/AI/BuiltInAI.h
    Source/AI/InstanceScript.h
    Source/AI/InstanceScripts.cpp
    Source/AI/SummonList.h
)

source_group("Scripts" FILES
    Source/Scripts/ShadowLordAI.h
)

source_group("Scripting" FILES
    Source/Scripting/IEntity.h
    Source/Scripting/IMapContext.h
    Source/Scripting/GameObjectScript.h
    Source/Scripting/GameObjectScripts.cpp
    Source/Scripting/QuestScript.h
    Source/Scripting/QuestScripts.cpp
    Source/Scripting/SpellScript.h
    Source/Scripting/SpellScripts.cpp
    Source/Scripting/PlayerScript.h
    Source/Scripting/PlayerScripts.cpp
)

source_group("Map" FILES
    Source/Map/MapDefines.h
    Source/Map/MapDefines.cpp
    Source/Map/MapInstance.h
    Source/Map/MapInstance.cpp
    Source/Map/MapManager.h
    Source/Map/MapManager.cpp
)

source_group("Grid" FILES
    Source/Grid/GridDefines.h
    Source/Grid/GridCell.h
    Source/Grid/Grid.h
    Source/Grid/Grid.cpp
)

source_group("Triggers" FILES
    Source/Triggers/TriggerScript.h
    Source/Triggers/TriggerScripts.cpp
)
