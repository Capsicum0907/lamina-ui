cmake_minimum_required(VERSION 3.25)
project(lamina-ui VERSION 0.0.0 LANGUAGES CXX)

option(LAMINA_BUILD_TESTS "Build the unit tests" OFF)

if(LAMINA_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(third_party/material-color-utilities)

add_library(lamina STATIC
    src/lamina/color/spec_2021.cpp
    src/lamina/motion/spring.cpp
)
add_library(lamina::lamina ALIAS lamina)
target_include_directories(lamina PUBLIC src)
target_compile_features(lamina PUBLIC cxx_std_20)
target_link_libraries(lamina PRIVATE mcu::mcu)
if(MSVC)
    target_compile_options(lamina PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(lamina PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(LAMINA_BUILD_TESTS)
    find_package(doctest CONFIG REQUIRED)
    add_executable(lamina_tests
        tests/color_scheme_test.cpp
        tests/main.cpp
        tests/motion_tokens_test.cpp
        tests/spring_test.cpp
        tests/tokens_test.cpp
    )
    target_link_libraries(lamina_tests PRIVATE lamina mcu::mcu doctest::doctest)
    if(MSVC)
        target_compile_options(lamina_tests PRIVATE /utf-8 /W4 /permissive-)
    endif()
    add_test(NAME lamina_tests COMMAND lamina_tests)
endif()
