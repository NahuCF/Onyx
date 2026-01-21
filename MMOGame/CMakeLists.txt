# MMO Game Project
project(MMOGame)

# ========== External Dependencies ==========

# ENet (networking)
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.18
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(enet)

# libpqxx (PostgreSQL C++ client) - for servers only
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBPQXX libpqxx)
endif()

# OpenSSL for password hashing
find_package(OpenSSL)

# ========== Shared Library ==========
add_subdirectory(Shared)

# ========== Server Projects ==========
# Only build servers if we have required dependencies
if(LIBPQXX_FOUND AND OPENSSL_FOUND)
    add_subdirectory(LoginServer)
    add_subdirectory(WorldServer)
    message(STATUS "LoginServer and WorldServer will be built (libpqxx and OpenSSL found)")
else()
    message(WARNING "Servers will NOT be built (requires libpqxx and OpenSSL)")
    if(NOT LIBPQXX_FOUND)
        message(STATUS "  - libpqxx not found. Install with: sudo apt install libpqxx-dev")
    endif()
    if(NOT OPENSSL_FOUND)
        message(STATUS "  - OpenSSL not found. Install with: sudo apt install libssl-dev")
    endif()
endif()

# ========== Client Project ==========
add_subdirectory(Client)

# ========== Editor Project ==========
add_subdirectory(Editor)

# ========== Editor 3D Project ==========
add_subdirectory(Editor3D)
