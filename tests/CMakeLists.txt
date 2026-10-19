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
    cmd/drawing_aids_test.cpp
    cmd/erase_command_test.cpp
    cmd/pline_command_test.cpp
    cmd/selection_set_test.cpp
    cmd/value_format_test.cpp
    core/arc_geometry_test.cpp
    core/arc_test.cpp
    core/circle_test.cpp
    core/document_query_test.cpp
    core/document_test.cpp
    core/geometry_test.cpp
    core/line_test.cpp
    core/polyline_test.cpp
    core/proximity_test.cpp
    core/snap_points_test.cpp
    core/spatial_index_test.cpp
    edit/editor_test.cpp
    render/viewport_test.cpp
    script/case_check_test.cpp
    script/compare_test.cpp
    script/editor_names_test.cpp
    script/entity_reader_test.cpp
    script/level1_test.cpp
    script/number_format_test.cpp
    script/questions_test.cpp
    script/report_test.cpp
    script/script_test.cpp
)
target_link_libraries(tg_tests PRIVATE
    Tecnigrafo::cmd
    Tecnigrafo::edit
    Tecnigrafo::render
    Tecnigrafo::script
    GTest::gtest_main)
# Scripts and reports of the module's own tests (not verification cases),
# and a folder for the files that the tests of tecnigrafo-script write.
target_compile_definitions(tg_tests PRIVATE
    TG_SCRIPT_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/script/dati"
    TG_SCRIPT_TEST_OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/script-output")
tg_configure_target(tg_tests)

gtest_discover_tests(tg_tests DISCOVERY_MODE PRE_TEST)

# Verification cases of the control mode (docs/adr/0011), label "verifica".
add_subdirectory(verifica)

# Performance is measured manually in Release, never as a timing gate in ctest.
add_executable(tg_bench bench/selection_bench.cpp)
target_link_libraries(tg_bench PRIVATE Tecnigrafo::core Tecnigrafo::edit)
target_compile_definitions(tg_bench PRIVATE TG_BENCH_BUILD_TYPE="$<CONFIG>")
tg_configure_target(tg_bench)

# Widgets of the application, driven with the Qt Test module and no display
# (docs/adr/0005). Built only together with the application.
if(TG_BUILD_APP)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test Widgets)
    add_executable(tg_app_tests
        app/command_line_test.cpp
        app/drawing_aids_test.cpp
        app/drawing_test.cpp
        app/script_session_test.cpp
        app/selection_test.cpp
    )
    target_link_libraries(tg_app_tests PRIVATE tg_app_ui Qt6::Test Qt6::Widgets GTest::gtest)
    # Level 2 of the control mode: the scripts of module script's own tests,
    # a folder for what the tests write, the program and the test tool
    # confronta_immagini (tests/verifica).
    target_compile_definitions(tg_app_tests PRIVATE
        TG_SCRIPT_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/script/dati"
        TG_APP_TEST_OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/app-output"
        TG_TECNIGRAFO="$<TARGET_FILE:tecnigrafo>"
        TG_CONFRONTA_IMMAGINI="$<TARGET_FILE:confronta_immagini>")
    add_dependencies(tg_app_tests tecnigrafo confronta_immagini)
    tg_configure_target(tg_app_tests)
    # One test entry: listing the cases would already need a QApplication.
    add_test(NAME app_widgets COMMAND tg_app_tests)
    set_tests_properties(app_widgets PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 60)
endif()
