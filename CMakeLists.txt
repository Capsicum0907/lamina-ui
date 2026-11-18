cmake_minimum_required(VERSION 3.25)
project(lamina-ui VERSION 0.0.0 LANGUAGES CXX)

option(LAMINA_BUILD_TESTS "Build the unit tests" OFF)

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
    src/lamina/color/spec_2021.cpp
    src/lamina/motion/spring.cpp
    src/lamina/render/canvas.cpp
    src/lamina/render/geometry.cpp
    src/lamina/render/path.cpp
    src/lamina/text/font.cpp
    src/lamina/text/layout.cpp
    src/lamina/widgets/button.cpp
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

find_path(THORVG_INCLUDE_DIR thorvg.h PATH_SUFFIXES thorvg-1 REQUIRED)
lamina_find_vcpkg_library(THORVG_LIBRARY thorvg-1)
add_library(lamina_thorvg STATIC src/lamina/render/thorvg_canvas.cpp)
add_library(lamina::thorvg ALIAS lamina_thorvg)
target_include_directories(lamina_thorvg PRIVATE ${THORVG_INCLUDE_DIR})
target_link_libraries(lamina_thorvg PUBLIC lamina PRIVATE ${THORVG_LIBRARY})
lamina_warnings(lamina_thorvg)

find_package(SDL3 CONFIG REQUIRED)
add_library(lamina_sdl3 STATIC src/lamina/platform/window.cpp)
add_library(lamina::sdl3 ALIAS lamina_sdl3)
target_link_libraries(lamina_sdl3 PUBLIC lamina lamina_thorvg PRIVATE SDL3::SDL3)
lamina_warnings(lamina_sdl3)

option(LAMINA_BUILD_EXAMPLES "Build the examples" ON)
if(LAMINA_BUILD_EXAMPLES)
    add_executable(playground examples/playground/main.cpp)
    target_link_libraries(playground PRIVATE lamina lamina_sdl3)
    lamina_warnings(playground)
    add_executable(buttons examples/buttons/main.cpp)
    target_link_libraries(buttons PRIVATE lamina lamina_sdl3)
    lamina_warnings(buttons)
endif()

if(LAMINA_BUILD_TESTS)
    find_package(doctest CONFIG REQUIRED)
    add_executable(lamina_tests
        tests/button_test.cpp
        tests/canvas_test.cpp
        tests/color_scheme_test.cpp
        tests/main.cpp
        tests/motion_helpers_test.cpp
        tests/motion_tokens_test.cpp
        tests/spring_test.cpp
        tests/text_test.cpp
        tests/tokens_test.cpp
    )
    target_link_libraries(lamina_tests PRIVATE lamina lamina_thorvg mcu::mcu doctest::doctest)
    lamina_warnings(lamina_tests)
    target_compile_definitions(lamina_tests PRIVATE LAMINA_TEST_FONTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fonts")
    add_test(NAME lamina_tests COMMAND lamina_tests)
endif()
