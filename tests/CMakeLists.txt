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
    cmd/arc_command_test.cpp
    cmd/circle_command_test.cpp
    cmd/command_runner_test.cpp
    cmd/coordinate_input_test.cpp
    cmd/drawing_aids_test.cpp
    cmd/erase_command_test.cpp
    cmd/line_command_test.cpp
    cmd/pline_command_test.cpp
    cmd/selection_set_test.cpp
    cmd/text_command_test.cpp
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
    core/raw_data_test.cpp
    core/snap_points_test.cpp
    core/spatial_index_test.cpp
    core/text_metrics_test.cpp
    core/text_test.cpp
    edit/editor_test.cpp
    io/dxf_lwpolyline_test.cpp
    io/dxf_text_test.cpp
    io/dxf_read_test.cpp
    io/dxf_write_test.cpp
    io/dxf_groups_test.cpp
    io/encoding_test.cpp
    io/file_access_test.cpp
    io/zip_test.cpp
    io/cadx_test.cpp
    io/json_test.cpp
    render/scene_renderer_test.cpp
    render/viewport_test.cpp
    script/case_check_test.cpp
    script/compare_test.cpp
    script/editor_names_test.cpp
    script/entity_reader_test.cpp
    script/file_test.cpp
    script/level1_test.cpp
    script/number_format_test.cpp
    script/questions_test.cpp
    script/report_test.cpp
    script/script_test.cpp
)
target_link_libraries(tg_tests PRIVATE
    Tecnigrafo::cmd
    Tecnigrafo::edit
    Tecnigrafo::io
    Tecnigrafo::render
    Tecnigrafo::script
    GTest::gtest_main)
# Scripts and reports of the module's own tests (not verification cases),
# and a folder for the files that the tests of tecnigrafo-script write.
target_compile_definitions(tg_tests PRIVATE
    TG_SCRIPT_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/script/dati"
    TG_SCRIPT_TEST_OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/script-output"
    TG_IO_TEST_OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/io-output"
    TG_IO_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/io/dati")
# The ZIP container is internal to io (io/src/zip.hpp); its tests reach it here.
target_include_directories(tg_tests PRIVATE "${PROJECT_SOURCE_DIR}/io/src")
tg_configure_target(tg_tests)

gtest_discover_tests(tg_tests DISCOVERY_MODE PRE_TEST)

# Verification cases of the control mode (docs/adr/0011), label "verifica".
add_subdirectory(verifica)

if(TG_FUZZ)
    add_subdirectory(fuzz)
endif()

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
        app/qt_painter_test.cpp
        app/script_session_test.cpp
        app/selection_test.cpp
        app/text_input_test.cpp
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

# Generate twice, independently of core's table, and compare byte for byte.
foreach(copy RANGE 1 2)
    add_test(NAME fontmetrics_generate_${copy}
        COMMAND fontmetrics "${PROJECT_SOURCE_DIR}/resources/fonts/LiberationSans-Regular.ttf"
                            "${CMAKE_CURRENT_BINARY_DIR}/font_metrics_${copy}.inc")
    set_tests_properties(fontmetrics_generate_${copy} PROPERTIES FIXTURES_SETUP fontmetrics_generated)
endforeach()
add_test(NAME fontmetrics_reproducible
    COMMAND ${CMAKE_COMMAND} -E compare_files
        "${CMAKE_CURRENT_BINARY_DIR}/font_metrics_1.inc"
        "${CMAKE_CURRENT_BINARY_DIR}/font_metrics_2.inc")
set_tests_properties(fontmetrics_reproducible PROPERTIES FIXTURES_REQUIRED fontmetrics_generated)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/truncated.ttf" CONTENT "invalid")
add_test(NAME fontmetrics_rejects_truncated
    COMMAND fontmetrics "${CMAKE_CURRENT_BINARY_DIR}/truncated.ttf"
                        "${CMAKE_CURRENT_BINARY_DIR}/invalid.inc")
set_tests_properties(fontmetrics_rejects_truncated PROPERTIES WILL_FAIL TRUE)
