cmake_minimum_required(VERSION 3.13)

project(cache-2Q)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable AddressSanitizer for Debug builds" OFF)

add_subdirectory(modules)

add_subdirectory(Tests)
