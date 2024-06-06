pkg_check_modules(PKG_CURL REQUIRED IMPORTED_TARGET libcurl)

add_executable(
        player
        main.cpp
        receive_from_server.cpp
        display_to_screen.cpp
        safe_queue.cpp
)
target_link_libraries(
        player PRIVATE
        PkgConfig::PKG_FFMPEG
        spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>
        PkgConfig::PKG_CURL
        nlohmann_json::nlohmann_json
        PkgConfig::PKG_SDL2
)