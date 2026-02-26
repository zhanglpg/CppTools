cmake_minimum_required(VERSION 3.14)
project(CppTools CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Header-only interface library so every target can include commonlibs/
add_library(commonlibs INTERFACE)
target_include_directories(commonlibs INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Optional: Boost (required only for Kalman / matrixinversion tests)
find_package(Boost QUIET COMPONENTS system)
if(Boost_FOUND)
    message(STATUS "Boost found at ${Boost_INCLUDE_DIRS} — Boost-dependent tests enabled")
    target_include_directories(commonlibs INTERFACE ${Boost_INCLUDE_DIRS})
else()
    message(STATUS "Boost not found — Boost-dependent tests will be skipped")
endif()

# GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Do not install GTest alongside the project
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)
