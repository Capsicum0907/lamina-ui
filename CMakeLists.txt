cmake_minimum_required(VERSION 3.25)
project(lamina-ui VERSION 0.0.0 LANGUAGES CXX)

option(LAMINA_BUILD_TESTS "Build the unit tests" OFF)
option(LAMINA_BUILD_THORVG "Build the ThorVG canvas (lamina::thorvg)" ON)
option(LAMINA_BUILD_SDL3 "Build the SDL3 window (lamina::sdl3)" ON)
option(LAMINA_BUILD_EXAMPLES "Build the examples" ON)
option(LAMINA_BUILD_TOOLS "Build the development tools" ON)

if(LAMINA_BUILD_EXAMPLES AND NOT (LAMINA_BUILD_SDL3 AND LAMINA_BUILD_THORVG))
    message(FATAL_ERROR "LAMINA_BUILD_EXAMPLES needs LAMINA_BUILD_SDL3 and LAMINA_BUILD_THORVG; turn LAMINA_BUILD_EXAMPLES off")
endif()
if(LAMINA_BUILD_TESTS AND NOT LAMINA_BUILD_THORVG)
    message(FATAL_ERROR "LAMINA_BUILD_TESTS needs LAMINA_BUILD_THORVG: the tests draw with the ThorVG canvas")
endif()

if(LAMINA_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(third_party/material-color-utilities)

function(lamina_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8 /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(lamina STATIC
    src/lamina/color/mcu_scheme.cpp
    src/lamina/color/spec_2021.cpp
    src/lamina/color/spec_2025.cpp
    src/lamina/icons/icon.cpp
    src/lamina/motion/spring.cpp
    src/lamina/render/canvas.cpp
    src/lamina/render/geometry.cpp
    src/lamina/render/path.cpp
    src/lamina/shapes/cubic.cpp
    src/lamina/shapes/feature.cpp
    src/lamina/shapes/feature_mapping.cpp
    src/lamina/shapes/float_mapping.cpp
    src/lamina/shapes/material_shapes.cpp
    src/lamina/shapes/morph.cpp
    src/lamina/shapes/point.cpp
    src/lamina/shapes/polygon_measure.cpp
    src/lamina/shapes/rounded_polygon.cpp
    src/lamina/shapes/shapes.cpp
    src/lamina/shapes/to_path.cpp
    src/lamina/text/font.cpp
    src/lamina/text/edit_buffer.cpp
    src/lamina/text/layout.cpp
    src/lamina/text/line_map.cpp
    src/lamina/text/word_boundary.cpp
    src/lamina/layout/layout.cpp
    src/lamina/widgets/button.cpp
    src/lamina/widgets/checkbox.cpp
    src/lamina/widgets/dialog.cpp
    src/lamina/widgets/fab.cpp
    src/lamina/widgets/focus_manager.cpp
    src/lamina/widgets/grouped_menu.cpp
    src/lamina/widgets/icon_button.cpp
    src/lamina/widgets/label.cpp
    src/lamina/widgets/menu.cpp
    src/lamina/widgets/navigation_bar.cpp
    src/lamina/widgets/navigation_rail.cpp
    src/lamina/widgets/overlay_stack.cpp
    src/lamina/widgets/progress_indicator.cpp
    src/lamina/widgets/radio_button.cpp
    src/lamina/widgets/scroll_area.cpp
    src/lamina/widgets/slider.cpp
    src/lamina/widgets/surface.cpp
    src/lamina/widgets/switch.cpp
    src/lamina/widgets/tabs.cpp
    src/lamina/widgets/text_field.cpp
)
add_library(lamina::lamina ALIAS lamina)
target_include_directories(lamina PUBLIC src)
target_compile_features(lamina PUBLIC cxx_std_20)
function(lamina_find_vcpkg_library var name)
    find_library(${var}_RELEASE NAMES ${name} PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH REQUIRED)
    find_library(${var}_DEBUG NAMES ${name} PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib" NO_DEFAULT_PATH REQUIRED)
    set(${var} optimized ${${var}_RELEASE} debug ${${var}_DEBUG} PARENT_SCOPE)
endfunction()

find_path(HARFBUZZ_INCLUDE_DIR hb.h PATH_SUFFIXES harfbuzz REQUIRED)
lamina_find_vcpkg_library(HARFBUZZ_LIBRARY harfbuzz)
target_include_directories(lamina PRIVATE ${HARFBUZZ_INCLUDE_DIR})
target_link_libraries(lamina PRIVATE mcu::mcu ${HARFBUZZ_LIBRARY})
lamina_warnings(lamina)

if(LAMINA_BUILD_THORVG)
    find_path(THORVG_INCLUDE_DIR thorvg.h PATH_SUFFIXES thorvg-1 REQUIRED)
    lamina_find_vcpkg_library(THORVG_LIBRARY thorvg-1)
    add_library(lamina_thorvg STATIC src/lamina_thorvg/canvas.cpp)
    add_library(lamina::thorvg ALIAS lamina_thorvg)
    target_include_directories(lamina_thorvg PRIVATE ${THORVG_INCLUDE_DIR})
    target_link_libraries(lamina_thorvg PUBLIC lamina PRIVATE ${THORVG_LIBRARY})
    lamina_warnings(lamina_thorvg)
endif()

if(LAMINA_BUILD_SDL3)
    find_package(SDL3 CONFIG REQUIRED)
    add_library(lamina_sdl3 STATIC
        src/lamina_sdl3/app.cpp
        src/lamina_sdl3/resources.cpp
        src/lamina_sdl3/system_fonts.cpp
        src/lamina_sdl3/window.cpp
    )
    add_library(lamina::sdl3 ALIAS lamina_sdl3)
    target_link_libraries(lamina_sdl3 PUBLIC lamina PRIVATE SDL3::SDL3)
    lamina_warnings(lamina_sdl3)
endif()

if(LAMINA_BUILD_EXAMPLES OR LAMINA_BUILD_TESTS)
    add_library(calculator_engine STATIC examples/calculator/engine.cpp examples/calculator/keypad_layout.cpp)
    target_include_directories(calculator_engine PUBLIC examples)
    target_compile_features(calculator_engine PUBLIC cxx_std_20)
    lamina_warnings(calculator_engine)
    add_library(timer_engine STATIC examples/timer/engine.cpp examples/timer/layout.cpp)
    target_include_directories(timer_engine PUBLIC examples)
    target_compile_features(timer_engine PUBLIC cxx_std_20)
    lamina_warnings(timer_engine)
endif()

function(lamina_add_example name dir)
    add_executable(${name} examples/${dir}/main.cpp)
    target_link_libraries(${name} PRIVATE lamina lamina_sdl3 lamina_examples_common)
    lamina_warnings(${name})
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${name}>/assets")
endfunction()

if(LAMINA_BUILD_EXAMPLES)
    add_library(lamina_examples_common STATIC
        examples/common/canvas.cpp
        examples/common/caption_cell.cpp
        examples/common/theme.cpp
    )
    target_include_directories(lamina_examples_common PUBLIC examples)
    target_link_libraries(lamina_examples_common PUBLIC lamina PRIVATE lamina_thorvg)
    lamina_warnings(lamina_examples_common)
    lamina_add_example(playground playground)
    lamina_add_example(buttons buttons)
    lamina_add_example(text_field text_field)
    lamina_add_example(shapes_gallery shapes)
    lamina_add_example(layout_demo layout)
    lamina_add_example(components components)
    lamina_add_example(navigation navigation)
    lamina_add_example(calculator calculator)
    target_sources(calculator PRIVATE examples/calculator/display.cpp)
    target_link_libraries(calculator PRIVATE calculator_engine)
    lamina_add_example(timer timer)
    target_sources(timer PRIVATE examples/timer/readout.cpp)
    target_link_libraries(timer PRIVATE timer_engine)
endif()

if(LAMINA_BUILD_TOOLS)
    add_executable(shape-dump tools/shape-dump/main.cpp)
    target_link_libraries(shape-dump PRIVATE lamina)
    lamina_warnings(shape-dump)
    add_executable(color-dump tools/color-dump/main.cpp)
    target_link_libraries(color-dump PRIVATE lamina)
    lamina_warnings(color-dump)
endif()

if(LAMINA_BUILD_TESTS)
    find_package(doctest CONFIG REQUIRED)
    add_executable(lamina_tests
        tests/animated_test.cpp
        tests/button_test.cpp
        tests/calculator_engine_test.cpp
        tests/calculator_layout_test.cpp
        tests/cancel_interaction_test.cpp
        tests/canvas_test.cpp
        tests/checkbox_test.cpp
        tests/color_mix_test.cpp
        tests/color_scheme_test.cpp
        tests/dialog_test.cpp
        tests/edit_buffer_test.cpp
        tests/focus_manager_test.cpp
        tests/font_ownership_test.cpp
        tests/grouped_menu_test.cpp
        tests/icon_button_test.cpp
        tests/icon_test.cpp
        tests/input_test.cpp
        tests/interaction_test.cpp
        tests/layout_test.cpp
        tests/lerp_test.cpp
        tests/main.cpp
        tests/menu_test.cpp
        tests/motion_tokens_test.cpp
        tests/navigation_bar_test.cpp
        tests/navigation_rail_test.cpp
        tests/overlay_stack_test.cpp
        tests/popup_transition_test.cpp
        tests/progress_indicator_test.cpp
        tests/protocol_test.cpp
        tests/radio_button_test.cpp
        tests/scroll_area_test.cpp
        tests/shapes_test.cpp
        tests/slider_test.cpp
        tests/spring_test.cpp
        tests/support.cpp
        tests/surface_test.cpp
        tests/switch_test.cpp
        tests/tabs_test.cpp
        tests/text_field_test.cpp
        tests/text_test.cpp
        tests/timer_engine_test.cpp
        tests/timer_layout_test.cpp
        tests/tokens_test.cpp
        tests/widget_set_test.cpp
        tests/word_boundary_test.cpp
    )
    target_link_libraries(lamina_tests PRIVATE lamina lamina_thorvg calculator_engine timer_engine mcu::mcu doctest::doctest)
    lamina_warnings(lamina_tests)
    target_compile_definitions(lamina_tests PRIVATE LAMINA_TEST_FONTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fonts")
    target_compile_definitions(lamina_tests PRIVATE LAMINA_ICONS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/icons")
    add_test(NAME lamina_tests COMMAND lamina_tests)
endif()
