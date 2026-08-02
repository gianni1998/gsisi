cmake_minimum_required(VERSION 3.10)

# Name of your project
project(gsisi LANGUAGES CXX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

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
        tests/ParserTests.cpp
        ${SOURCES}
    )

    add_executable(semantic_tests
        tests/SemanticAnalyserTests.cpp
    )

    add_executable(ir_generator_tests
        tests/IRGeneratorTests.cpp
    )

    add_executable(assembly_generator_tests
        tests/AssemblyGeneratorTests.cpp
    )

    target_include_directories(parser_tests PRIVATE include)
    target_link_libraries(semantic_tests PRIVATE gsisi_frontend)
    target_link_libraries(ir_generator_tests PRIVATE gsisi_frontend)
    target_link_libraries(assembly_generator_tests PRIVATE gsisi_frontend)

    add_test(
        NAME parser_tests
        COMMAND parser_tests
    )

    add_test(
        NAME semantic_tests
        COMMAND semantic_tests
    )

    add_test(
        NAME ir_generator_tests
        COMMAND ir_generator_tests
    )

    # add_test(
    #     NAME semantic_sample
    #     COMMAND si_compiler
    #         ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-2.si
    #         ${CMAKE_CURRENT_SOURCE_DIR}/build/test-2.asm
    # )

    # add_test(
    #     NAME compiler_ir_smoke_test
    #     COMMAND si_compiler
    #             ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-3.si
    #             ${CMAKE_CURRENT_SOURCE_DIR}/build/test-3.asm
    # )

    add_test(
        NAME assembly_generator_tests
        COMMAND assembly_generator_tests
    )

    find_program(
        NASM_EXECUTABLE
        nasm
        REQUIRED
    )

    find_program(
        LD_EXECUTABLE
        ld
        REQUIRED
    )

    add_test(
        NAME backend_native_smoke
        COMMAND
            bash
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_backend_smoke_test.sh
            $<TARGET_FILE:si_compiler>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-4.si
            ${CMAKE_CURRENT_BINARY_DIR}/backend-smoke
            ${NASM_EXECUTABLE}
            ${LD_EXECUTABLE}
            42
    )
endif()