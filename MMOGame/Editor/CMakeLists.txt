# MMO Editor
project(MMOEditor)

set(EDITOR_SOURCES
    Source/Main.cpp
    Source/EditorLayer.cpp
    Source/Tileset.cpp
    Source/TileMap.cpp
    Source/Chunk.cpp
    Source/FileDialog.cpp
    Source/TileMapRenderer.cpp
    Source/Panels/PanelManager.cpp
    Source/Panels/HierarchyPanel.cpp
    Source/Panels/PropertiesPanel.cpp
    Source/Panels/TilesetPanel.cpp
    Source/Panels/AssetBrowserPanel.cpp
    Source/Panels/ViewportPanel.cpp
    Source/Panels/StatisticsPanel.cpp
)

set(EDITOR_HEADERS
    Source/EditorLayer.h
    Source/Tileset.h
    Source/TileMap.h
    Source/Chunk.h
    Source/EditorDefines.h
    Source/FileDialog.h
    Source/TileMapRenderer.h
    Source/Panels/EditorContext.h
    Source/Panels/EditorPanel.h
    Source/Panels/PanelManager.h
    Source/Panels/HierarchyPanel.h
    Source/Panels/PropertiesPanel.h
    Source/Panels/TilesetPanel.h
    Source/Panels/AssetBrowserPanel.h
    Source/Panels/ViewportPanel.h
    Source/Panels/StatisticsPanel.h
)

add_executable(MMOEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(MMOEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Onyx
    ${CMAKE_SOURCE_DIR}/Onyx/Source
)

target_link_libraries(MMOEditor PRIVATE
    MMOShared
    Onyx
)

set_target_properties(MMOEditor PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "MMOEditor"
    FOLDER "MMO"
)

# Platform-specific settings
if(WIN32)
    set_target_properties(MMOEditor PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# Group source files for IDEs
source_group("Source Files" FILES ${EDITOR_SOURCES})
source_group("Header Files" FILES ${EDITOR_HEADERS})
source_group("Source Files\\Panels" FILES
    Source/Panels/PanelManager.cpp
    Source/Panels/HierarchyPanel.cpp
    Source/Panels/PropertiesPanel.cpp
    Source/Panels/TilesetPanel.cpp
    Source/Panels/AssetBrowserPanel.cpp
    Source/Panels/ViewportPanel.cpp
    Source/Panels/StatisticsPanel.cpp
)
source_group("Header Files\\Panels" FILES
    Source/Panels/EditorContext.h
    Source/Panels/EditorPanel.h
    Source/Panels/PanelManager.h
    Source/Panels/HierarchyPanel.h
    Source/Panels/PropertiesPanel.h
    Source/Panels/TilesetPanel.h
    Source/Panels/AssetBrowserPanel.h
    Source/Panels/ViewportPanel.h
    Source/Panels/StatisticsPanel.h
)
