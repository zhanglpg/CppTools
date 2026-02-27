# CLAUDE.md — CppTools

## Project Overview

CppTools is a header-only C++ utility library collection containing data structures, algorithms, networking primitives, signal processing, and system utilities. Originally developed around 2008–2009 using Boost, the codebase has been modernized to C++17 with standalone Asio (no Boost dependency).

## Build & Test Commands

```bash
# Configure and build
cmake -B build -S .
cmake --build build

# Run all tests
cd build && ctest --output-on-failure

# Run a single test
cd build && ctest -R test_segment_tree --output-on-failure
```

**Requirements**: CMake 3.14+, C++17 compiler (GCC or Clang), Linux/POSIX environment. Dependencies (Asio 1.28.2, GoogleTest 1.14.0) are fetched automatically via CMake FetchContent.

## Repository Structure

```
CppTools/
├── CMakeLists.txt           # Root build configuration
├── commonlibs/              # Header-only library (all .hpp files)
│   ├── dp/                  # Signal processing (Kalman filter, matrix ops)
│   │   ├── kalman.hpp
│   │   └── matrixinversion.hpp
│   ├── interproc/           # POSIX shared memory IPC
│   │   └── interp.hpp
│   ├── algorithms.hpp       # Graph algorithms (Dijkstra)
│   ├── connection.hpp       # TCP socket connection (Asio)
│   ├── connection_http.hpp  # HTTP client
│   ├── datagram.hpp         # UDP datagram
│   ├── prefix_tree.hpp      # Trie data structure
│   ├── segment_tree.hpp     # Segment tree (range queries)
│   ├── singleton.hpp        # Thread-safe Singleton (std::call_once)
│   ├── textlogger.hpp       # File logging with rotation
│   ├── posixtime_util.hpp   # Chrono-based time utilities
│   └── ...                  # Additional utility headers
└── tests/                   # GoogleTest unit tests
    ├── CMakeLists.txt       # Test targets (add_cpptools_test helper)
    └── test_*.cpp           # One test file per component (10 total)
```

## Code Conventions

- **Header-only**: All library code lives in `.hpp` files under `commonlibs/`. No `.cpp` source files in the library.
- **Namespace**: All code is in the `commonlibs::` namespace.
- **Include guards**: Use `#ifndef`/`#define`/`#endif` (not `#pragma once`).
- **Include style**: Headers are included as `#include "commonlibs/header.hpp"` from the project root.
- **C++ standard**: C++17. Use `std::` equivalents over Boost (e.g., `std::chrono`, `std::filesystem`, `std::optional`).
- **Templates**: Heavy use of templates and CRTP for compile-time polymorphism.
- **Compiler warnings**: Code must compile cleanly with `-Wall -Wextra -Wpedantic`.

## Testing

- **Framework**: GoogleTest (GTest), linked via `GTest::gtest_main`.
- **Adding a test**: Use the `add_cpptools_test(target source)` helper in `tests/CMakeLists.txt`. Each test file should correspond to a single library header.
- **Naming**: Test files follow the pattern `test_<component>.cpp`.
- **Test coverage**: 10 test executables covering algorithms, data structures, signal processing, utilities, and IPC.

## Key Design Patterns

- **Singleton**: Thread-safe via `std::call_once` and `std::unique_ptr` (see `singleton.hpp`).
- **CRTP**: Used for `errorStatus_Saver<T>` and `dp_kalman<Tfloat, Timp>`.
- **Asio networking**: Standalone Asio (non-Boost) with `ASIO_STANDALONE` defined. TCP connections, UDP datagrams, and HTTP client are template-based.
- **IPC**: POSIX `shm_open()`/`mmap()` via templated wrappers in `interproc/interp.hpp`. Requires linking with `-lrt` on Linux.

## Dependencies

| Dependency   | Version | Source        | Purpose                    |
|-------------|---------|---------------|----------------------------|
| Asio        | 1.28.2  | FetchContent  | Networking (TCP/UDP/HTTP)  |
| GoogleTest  | 1.14.0  | FetchContent  | Unit testing               |
| POSIX (`rt`)| System  | System        | Shared memory IPC          |
| Threads     | System  | System        | Thread support (pthread)   |

No external package manager (vcpkg, conan) is used. All third-party dependencies are pulled at configure time by CMake FetchContent.
