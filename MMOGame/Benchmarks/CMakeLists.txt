project(MMOBenchmarks)

add_executable(OmdlVsFbx OmdlVsFbx.cpp)

target_link_libraries(OmdlVsFbx PRIVATE MMOShared assimp)

set_target_properties(OmdlVsFbx PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    FOLDER "MMO"
)
