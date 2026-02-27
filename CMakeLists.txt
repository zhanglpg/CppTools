cmake_minimum_required(VERSION 3.14)
project(CppTools CXX)

set(CMAKE_CXX_STANDARD 17)
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

# Standalone Asio (header-only, no Boost required)
include(FetchContent)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-28-2
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()
add_library(asio_headers INTERFACE)
target_include_directories(asio_headers INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio_headers INTERFACE ASIO_STANDALONE)

target_link_libraries(commonlibs INTERFACE asio_headers)

# On Linux, POSIX shared memory needs -lrt and networking needs -lpthread
if(UNIX)
    target_link_libraries(commonlibs INTERFACE rt)
    find_package(Threads REQUIRED)
    target_link_libraries(commonlibs INTERFACE Threads::Threads)
endif()

# GoogleTest via FetchContent
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
