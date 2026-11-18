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
)
add_library(lamina::lamina ALIAS lamina)
target_include_directories(lamina PUBLIC src)
target_compile_features(lamina PUBLIC cxx_std_20)
target_link_libraries(lamina PRIVATE mcu::mcu)
lamina_warnings(lamina)

find_path(THORVG_INCLUDE_DIR thorvg.h PATH_SUFFIXES thorvg-1 REQUIRED)
find_library(THORVG_LIBRARY_RELEASE NAMES thorvg-1 thorvg PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH REQUIRED)
find_library(THORVG_LIBRARY_DEBUG NAMES thorvg-1 thorvg PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib" NO_DEFAULT_PATH REQUIRED)
set(THORVG_LIBRARY optimized ${THORVG_LIBRARY_RELEASE} debug ${THORVG_LIBRARY_DEBUG})
add_library(lamina_thorvg STATIC src/lamina/render/thorvg_canvas.cpp)
add_library(lamina::thorvg ALIAS lamina_thorvg)
target_include_directories(lamina_thorvg PRIVATE ${THORVG_INCLUDE_DIR})
target_link_libraries(lamina_thorvg PUBLIC lamina PRIVATE ${THORVG_LIBRARY})
lamina_warnings(lamina_thorvg)

if(LAMINA_BUILD_TESTS)
    find_package(doctest CONFIG REQUIRED)
    add_executable(lamina_tests
        tests/canvas_test.cpp
        tests/color_scheme_test.cpp
        tests/main.cpp
        tests/motion_tokens_test.cpp
        tests/spring_test.cpp
        tests/tokens_test.cpp
    )
    target_link_libraries(lamina_tests PRIVATE lamina lamina_thorvg mcu::mcu doctest::doctest)
    lamina_warnings(lamina_tests)
    add_test(NAME lamina_tests COMMAND lamina_tests)
endif()
