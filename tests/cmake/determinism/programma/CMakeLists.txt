# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The Tecnigrafo authors

# A target in a subdirectory, and a directory option that arrives after it:
# only the check of the directory sees that one.
add_executable(probe_program ../probe.cpp)
if(TG_PROBE_DIRECTORY_OPTION)
    add_compile_options("${TG_PROBE_DIRECTORY_OPTION}")
endif()
