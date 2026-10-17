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
    cmd/selection_set_test.cpp
    core/document_query_test.cpp
    core/document_test.cpp
    core/geometry_test.cpp
    core/polyline_test.cpp
    core/proximity_test.cpp
    core/snap_points_test.cpp
    core/spatial_index_test.cpp
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

# Widgets of the application, driven with the Qt Test module and no display
# (docs/adr/0005). Built only together with the application.
if(TG_BUILD_APP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test Widgets)
    add_executable(tg_app_tests
        app/command_line_test.cpp
        app/drawing_test.cpp
    )
    target_link_libraries(tg_app_tests PRIVATE tg_app_ui Qt6::Test Qt6::Widgets GTest::gtest)
    tg_configure_target(tg_app_tests)
    # One test entry: listing the cases would already need a QApplication.
    add_test(NAME app_widgets COMMAND tg_app_tests)
    set_tests_properties(app_widgets PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 60)
endif()
