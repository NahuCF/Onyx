# MMO Editor 3D
project(MMOEditor3D)

# ImGuiFileDialog - Cross-platform file dialog for ImGui
FetchContent_Declare(
    ImGuiFileDialog
    GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
    GIT_TAG        v0.6.7
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(ImGuiFileDialog)

# nlohmann/json is fetched in MMOGame/CMakeLists.txt so all subprojects share it.

# ImGuiFileDialog needs imgui headers
target_include_directories(ImGuiFileDialog PUBLIC ${imgui_SOURCE_DIR})

set(EDITOR3D_SOURCES
    Source/Main.cpp
    Source/Editor3DLayer.cpp
    Source/Panels/ViewportPanel.cpp
    Source/Panels/HierarchyPanel.cpp
    Source/Panels/InspectorPanel.cpp
    Source/Panels/AssetBrowserPanel.cpp
    Source/Panels/StatisticsPanel.cpp
    Source/Panels/LightingPanel.cpp
    Source/Panels/TerrainPanel.cpp
    Source/World/EditorWorld.cpp
    Source/Gizmo/TransformGizmo.cpp
    Source/Commands/EditorCommand.cpp
    Source/Terrain/TerrainChunk.cpp
    Source/World/WorldChunk.cpp
    Source/World/EditorWorldSystem.cpp
    Source/Terrain/MaterialSerializer.cpp
    Source/Terrain/TerrainMaterialLibrary.cpp
    Source/Panels/MaterialEditorPanel.cpp
    Source/Map/EditorMapRegistry.cpp
    Source/Map/MapBrowserDialog.cpp
    Source/Data/RaceClassRegistry.cpp
    Source/Export/MigrationSqlWriter.cpp
    Source/Runtime/Subprocess.cpp
    Source/Runtime/LocalRunSession.cpp
    Source/Settings/EditorPreferences.cpp
    Source/Navigation/NavMeshBuilder.cpp
    Source/Navigation/NavMeshBakeService.cpp
    Source/Navigation/NavMeshDebugView.cpp
)

set(EDITOR3D_HEADERS
    Source/Editor3DLayer.h
    Source/Panels/ViewportPanel.h
    Source/Panels/HierarchyPanel.h
    Source/Panels/InspectorPanel.h
    Source/Panels/AssetBrowserPanel.h
    Source/Panels/StatisticsPanel.h
    Source/Panels/LightingPanel.h
    Source/Panels/TerrainPanel.h
    Source/World/EditorWorld.h
    Source/Gizmo/TransformGizmo.h
    Source/Commands/EditorCommand.h
    Source/Rendering/EditorVisuals.h
    Source/Terrain/TerrainChunk.h
    Source/World/WorldChunk.h
    Source/World/EditorWorldSystem.h
    Source/Terrain/MaterialSerializer.h
    Source/Terrain/TerrainMaterialLibrary.h
    Source/Panels/MaterialEditorPanel.h
    Source/Map/EditorMapRegistry.h
    Source/Map/MapBrowserDialog.h
    Source/Data/RaceClassRegistry.h
    Source/Export/MigrationSqlWriter.h
    Source/Runtime/Subprocess.h
    Source/Runtime/LocalRunSession.h
    Source/Settings/EditorPreferences.h
    Source/Navigation/NavMeshBuilder.h
    Source/Navigation/NavMeshBakeService.h
    Source/Navigation/NavMeshDebugView.h
)

add_executable(MMOEditor3D ${EDITOR3D_SOURCES} ${EDITOR3D_HEADERS})

target_include_directories(MMOEditor3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Onyx
    ${CMAKE_SOURCE_DIR}/Onyx/Source
    ${CMAKE_BINARY_DIR}/_deps/assimp-src/include
    ${CMAKE_BINARY_DIR}/_deps/assimp-build/include
    ${ImGuiFileDialog_SOURCE_DIR}
)

target_link_libraries(MMOEditor3D PRIVATE
    Onyx
    MMOShared
    assimp
    ImGuiFileDialog
    nlohmann_json::nlohmann_json
    Recast
    Detour
)

set_target_properties(MMOEditor3D PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MMOEditor3D"
    FOLDER "MMO"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Platform-specific settings
if(WIN32)
    set_target_properties(MMOEditor3D PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

source_group("Source Files" FILES ${EDITOR3D_SOURCES})
source_group("Header Files" FILES ${EDITOR3D_HEADERS})
