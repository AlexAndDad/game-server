cmake_minimum_required(VERSION 3.9)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(toolchain)
include(HunterGate)

message(STATUS "[project] CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")


HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.20.21.tar.gz"
        SHA1 "36bc6c263eb173697724e00adac8add4b3e52a31"
)

project(GameServer)

hunter_add_package(sugar)
find_package(sugar CONFIG REQUIRED)

sugar_include(src)
add_executable(game-server ${SRC_FILES})
target_include_directories(game-server PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(AddBoost)
AddBoost(game-server COMPONENTS system log thread)
