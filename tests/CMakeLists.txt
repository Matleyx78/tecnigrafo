# SPDX-License-Identifier: GPL-3.0-or-later

# Use an installed GoogleTest when available, otherwise fetch a pinned
# release. The fetched version must match docs/adr/0002.
find_package(GTest 1.14 QUIET)
if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
        GIT_SHALLOW TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(tg_tests
    cmd/command_runner_test.cpp
    cmd/coordinate_input_test.cpp
    cmd/pline_command_test.cpp
    core/document_test.cpp
    core/geometry_test.cpp
    core/polyline_test.cpp
    edit/editor_test.cpp
    render/viewport_test.cpp
)
target_link_libraries(tg_tests PRIVATE
    Tecnigrafo::cmd
    Tecnigrafo::edit
    Tecnigrafo::render
    GTest::gtest_main)
tg_configure_target(tg_tests)

gtest_discover_tests(tg_tests DISCOVERY_MODE PRE_TEST)
