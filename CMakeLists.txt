cmake_minimum_required(VERSION 3.25)
project(client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

include(ExternalProject)
include(FetchContent)
find_package(PkgConfig REQUIRED)

set(EP_BASE ${CMAKE_CURRENT_BINARY_DIR}/external)
set(EP_BUILD_ALWAYS 0L)
set(EP_J 36)

set_directory_properties(PROPERTIES EP_BASE "${EP_BASE}")

# ======== build nasm ========
configure_file(${CMAKE_SOURCE_DIR}/nasm/CMakeLists.txt.in ${CMAKE_SOURCE_DIR}/nasm/CMakeLists.txt @ONLY)

execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . -B ${CMAKE_BINARY_DIR}/nasm
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/nasm
)

execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/nasm
)

# ======== build ffmpeg ========
configure_file(${CMAKE_SOURCE_DIR}/ffmpeg/CMakeLists.txt.in ${CMAKE_SOURCE_DIR}/ffmpeg/CMakeLists.txt @ONLY)

execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . -B ${CMAKE_BINARY_DIR}/ffmpeg
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ffmpeg
)

execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ffmpeg
)

set(ENV{PKG_CONFIG_PATH} "${EP_BASE}/Install/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(PKG_FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavdevice libavfilter libavformat libavutil libpostproc libswresample libswscale)

# ======== build spdlog ========
configure_file(${CMAKE_SOURCE_DIR}/spdlog/CMakeLists.txt.in ${CMAKE_SOURCE_DIR}/spdlog/CMakeLists.txt @ONLY)

execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . -B ${CMAKE_BINARY_DIR}/spdlog
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/spdlog
)

execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/spdlog
)

find_package(spdlog REQUIRED PATHS ${EP_BASE}/Install/lib/cmake NO_DEFAULT_PATH)

# ======== build nlohmann/json ========
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# ======== setup SDL2 ========
pkg_check_modules(PKG_SDL2 REQUIRED IMPORTED_TARGET sdl2)

# ======== build player_old ========

message("${PKG_FFMPEG_LINK_LIBRARIES}")

add_subdirectory(player)