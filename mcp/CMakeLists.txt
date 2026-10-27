# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors
#
# tg_mcp: the development MCP server over stdio (docs/adr/0027). May depend
# on tg_script, tg_cmd, tg_io and nlohmann/json only, never on Qt
# (docs/adr/0001). No network port and no socket (docs/adr/0011, point 10).

add_library(tg_mcp
    src/command_tools.cpp
    src/server.cpp
)
add_library(Tecnigrafo::mcp ALIAS tg_mcp)

target_include_directories(tg_mcp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
# nlohmann/json is public: Tool::inputSchema() returns its type.
target_link_libraries(tg_mcp PUBLIC Tecnigrafo::script Tecnigrafo::cmd Tecnigrafo::io tg_nlohmann_json)
# The version that initialize and server/discover report.
target_compile_definitions(tg_mcp PRIVATE TG_VERSION="${PROJECT_VERSION}")
tg_configure_target(tg_mcp)

# tecnigrafo-mcp: a development tool for agents, built with the tests and
# never installed with the program (docs/adr/0027, point 1).
add_executable(tecnigrafo-mcp tool/main.cpp)
target_link_libraries(tecnigrafo-mcp PRIVATE Tecnigrafo::mcp)
target_compile_definitions(tecnigrafo-mcp PRIVATE TG_VERSION="${PROJECT_VERSION}")
tg_configure_target(tecnigrafo-mcp)
