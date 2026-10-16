# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_configure_target(<target>)
# Applies the project-wide warning level (and optionally clang-tidy) to a
# target. Every first-party target must call it.

function(tg_configure_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(TG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
            -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual
            -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough)
        if(TG_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if(TG_CLANG_TIDY_COMMAND)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${TG_CLANG_TIDY_COMMAND}")
    endif()
endfunction()
