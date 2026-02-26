cmake_minimum_required(VERSION 3.14)
project(CppTools CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Optional: address/UB sanitizers (enable with -DSANITIZE=ON)
option(SANITIZE "Build with AddressSanitizer + UBSan" OFF)
if(SANITIZE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Header-only interface library so every target can include commonlibs/
add_library(commonlibs INTERFACE)
target_include_directories(commonlibs INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# Boost: retain only components still used after modernisation.
#   - system      : required by Boost.ASIO
#   - thread      : required by Boost.ASIO on some platforms
#   - interprocess: used by commonlibs/interproc/
#   - numeric     : uBLAS, used by matrixinversion / kalman
# Removed (replaced by std): date_time, signals, foreach, lexical_cast
find_package(Boost QUIET COMPONENTS system thread)
if(Boost_FOUND)
    message(STATUS "Boost found at ${Boost_INCLUDE_DIRS} — Boost-dependent tests enabled")
    target_include_directories(commonlibs INTERFACE ${Boost_INCLUDE_DIRS})
    target_link_libraries(commonlibs INTERFACE ${Boost_LIBRARIES})
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
