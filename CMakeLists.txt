cmake_minimum_required(VERSION 3.16)
project(PuzzleAdaptive VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Backend
add_subdirectory(backend)

# Tests
enable_testing()
add_subdirectory(tests)