cmake_minimum_required(VERSION 3.10)

# Name of your project
project(gsisi LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
file(GLOB TEST_SOURCES "tests/*.cpp")

add_library(gsisi_frontend ${SOURCES})

target_include_directories(gsisi_frontend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(si_compiler src/main.cpp)

target_link_libraries(si_compiler
    PRIVATE
        gsisi_frontend
)

# --------------------------------------------------
# Tests
# --------------------------------------------------
option(BUILD_TESTING "Build the gsisi tests" ON)

if(BUILD_TESTING)
    enable_testing()

    add_executable(parser_tests
        ${TEST_SOURCES}
        ${SOURCES}
    )

    target_include_directories(parser_tests PRIVATE include)

    add_test(
        NAME parser_tests
        COMMAND parser_tests
    )
endif()