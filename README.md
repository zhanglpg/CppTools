# CppTools

A header-only C++ utility library collection containing data structures, algorithms, networking primitives, signal processing, and system utilities. Originally developed around 2008–2009 using Boost, the codebase has been modernized to **C++17** with standalone Asio (no Boost dependency).

## Features

### Data Structures
- **Prefix Tree (Trie)** — Efficient string storage and retrieval
- **Segment Tree** — Range query operations with O(log n) updates
- **Proto Array** — Preallocated array with move semantics

### Algorithms
- **Graph Algorithms** — Dijkstra's shortest path, topological sorting
- **Signal Processing** — Kalman filter, matrix operations (in `dp/`)

### Networking
- **TCP Connection** — Template-based socket connection using Asio (`connection.hpp`, `connection_new.hpp`)
- **HTTP Client** — Simple HTTP request/response handling
- **UDP Datagram** — Connectionless messaging

### System Utilities
- **Singleton** — Thread-safe singleton pattern using `std::call_once`
- **Text Logger** — File logging with rotation support
- **POSIX Time Utilities** — Chrono-based time manipulation
- **Signal Handler** — Unix signal handling
- **Ctrl+C Handler** — SIGINT interception via singleton
- **Sleep Relative** — Chrono-based relative sleep and system time helpers
- **Error Status** — CRTP-based error status tracking with common error codes
- **Process ID** — Current process identification
- **Unique Ptr Macro** — Legacy compatibility alias for `std::unique_ptr`

### Inter-Process Communication
- **Shared Memory** — POSIX `shm_open()`/`mmap()` wrappers (in `interproc/`)

## Build & Test

```bash
# Configure and build
cmake -B build -S .
cmake --build build

# Run all tests
cd build && ctest --output-on-failure

# Run a single test
cd build && ctest -R test_segment_tree --output-on-failure
```

**Requirements:**
- CMake 3.14+
- C++17 compiler (GCC or Clang)
- Linux/POSIX environment

Dependencies (Asio 1.28.2, GoogleTest 1.14.0) are fetched automatically via CMake FetchContent.

## Repository Structure

```
CppTools/
├── CMakeLists.txt           # Root build configuration
├── commonlibs/              # Header-only library (all .hpp files)
│   ├── dp/                  # Signal processing
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
│   ├── singleton.hpp        # Thread-safe Singleton
│   ├── textlogger.hpp       # File logging with rotation
│   ├── posixtime_util.hpp   # Chrono-based time utilities
│   └── ...                  # Additional utility headers
└── tests/                   # GoogleTest unit tests
    ├── CMakeLists.txt
    └── test_*.cpp           # One test file per component
```

## Usage Examples

### Singleton Pattern

```cpp
#include "commonlibs/singleton.hpp"

class MyService : public commonlibs::Singleton<MyService> {
public:
    void doWork() { /* ... */ }
};

// Usage
MyService::instance().doWork();
```

### TCP Connection

```cpp
#include "commonlibs/connection.hpp"
#include <asio.hpp>

commonlibs::Connection<> conn;
conn.connect("localhost", 8080);
conn.send("Hello, server!");
auto response = conn.receive();
```

### Text Logger

```cpp
#include "commonlibs/textlogger.hpp"

commonlibs::TextLogger logger("app.log");
logger.log(commonlibs::LogLevel::INFO, "Application started");
logger.log(commonlibs::LogLevel::ERROR, "Something went wrong");
```

### Kalman Filter

```cpp
#include "commonlibs/dp/kalman.hpp"

commonlibs::dp::Kalman<float> kalman;
kalman.init(/* parameters */);
float estimate = kalman.filter(measurement);
```

## Code Conventions

- **Header-only**: All library code lives in `.hpp` files under `commonlibs/`
- **Namespace**: All code is in the `commonlibs::` namespace
- **Include guards**: Use `#ifndef`/`#define`/`#endif` (not `#pragma once`)
- **Include style**: `#include "commonlibs/header.hpp"` from project root
- **C++ standard**: C++17 — use `std::` equivalents over Boost
- **Templates**: Heavy use of templates and CRTP for compile-time polymorphism

## Dependencies

| Dependency   | Version | Source        | Purpose                    |
|-------------|---------|---------------|----------------------------|
| Asio        | 1.28.2  | FetchContent  | Networking (TCP/UDP/HTTP)  |
| GoogleTest  | 1.14.0  | FetchContent  | Unit testing               |
| POSIX (`rt`)| System  | System        | Shared memory IPC          |
| Threads     | System  | System        | Thread support (pthread)   |

No external package manager (vcpkg, conan) is used. All third-party dependencies are pulled at configure time by CMake FetchContent.

## Testing

- **Framework**: GoogleTest (GTest)
- **Test Coverage**: 14 test executables covering algorithms, data structures, signal processing, networking, utilities, and IPC
- **Adding Tests**: Use the `add_cpptools_test(target source)` helper in `tests/CMakeLists.txt`

## Key Design Patterns

- **Singleton**: Thread-safe via `std::call_once` and `std::unique_ptr`
- **CRTP**: Used for `errorStatus_Saver<T>` and `dp_kalman<Tfloat, Timp>`
- **Asio Networking**: Standalone Asio (non-Boost) with `ASIO_STANDALONE` defined
- **IPC**: POSIX `shm_open()`/`mmap()` via templated wrappers

## License

MIT (modernized code) / GPL v2 (legacy scripts) — see individual file headers for details.

## History

Originally developed 2008–2009 using Boost C++ libraries. Modernized in 2026 to C++17 with standalone Asio, removing all Boost dependencies while preserving the original API design.
