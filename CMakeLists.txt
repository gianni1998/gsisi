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