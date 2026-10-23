# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_io: reading and writing drawings (DXF, .cadx). Depends only on core;
# no Qt (docs/adr/0017). The only third-party library is zlib, linked
# privately for the ZIP container (docs/adr/0019).

add_library(tg_io
    src/dxf_groups.cpp
    src/encoding.cpp
    src/file_access.cpp
    src/io.cpp
    src/zip.cpp
)
add_library(Tecnigrafo::io ALIAS tg_io)

target_include_directories(tg_io PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_io PUBLIC Tecnigrafo::core PRIVATE tg_zlib)
target_compile_features(tg_io PUBLIC cxx_std_20)
tg_configure_target(tg_io)
