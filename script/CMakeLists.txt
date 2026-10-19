# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_script: the control mode at level 1 (docs/adr/0011): scripts, reports,
# questions to the model and their comparison with expected results.
# May depend on tg_core, tg_edit and tg_cmd only, never on Qt
# (docs/adr/0001).

add_library(tg_script
    src/case_check.cpp
    src/compare.cpp
    src/construction_questions.cpp
    src/entity_reader.cpp
    src/level1.cpp
    src/number_format.cpp
    src/question_table.cpp
    src/questions.cpp
    src/report.cpp
    src/script.cpp
    src/text_format.cpp
)
add_library(Tecnigrafo::script ALIAS tg_script)

target_include_directories(tg_script PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_script PUBLIC Tecnigrafo::cmd)
tg_configure_target(tg_script)

# tecnigrafo-script: a development tool for agents and CI, never installed
# with the program (docs/adr/0011, point 10).
add_executable(tecnigrafo-script tool/main.cpp)
target_link_libraries(tecnigrafo-script PRIVATE Tecnigrafo::script)
target_compile_definitions(tecnigrafo-script PRIVATE TG_VERSION="${PROJECT_VERSION}")
tg_configure_target(tecnigrafo-script)
