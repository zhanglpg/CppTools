# C++ Modernization Plan

## Goal

Migrate the codebase from pre-C++11/Boost-heavy code to clean C++17.
Boost will be **fully removed** from all utility headers. It is intentionally
**retained** for three headers where no standard equivalent exists:
- `boost::asio` (networking — not yet standardized)
- `boost::interprocess` (IPC — no standard equivalent)
- `boost::numeric::ublas` (matrix algebra — would require Eigen, a separate decision)

The CMake build will be updated to reflect the reduced Boost surface.

---

## Phase 1 — Mechanical / Quick Wins (no logic changes)

### 1.1 Delete `uniqueptr.hpp`
**File:** `commonlibs/uniqueptr.hpp`

This file is purely a C++03/C++11 compatibility shim that conditionally
aliases `std::unique_ptr` or `boost::scoped_ptr` behind a macro. With C++17
as our baseline, `std::unique_ptr` is always available.

**Changes:**
- Delete the file entirely.
- Remove its `#include` from `connection.hpp` and `connection_new.hpp`.
- Replace every occurrence of the shim typedef with `std::unique_ptr`.

---

### 1.2 Replace `NULL` with `nullptr` everywhere
**Files:** `interproc/interp.hpp`, `prefix_tree.hpp`, `segment_tree.hpp`,
`currentprocessid.hpp`

Pure mechanical replacement; `nullptr` is typed and avoids implicit-int
conversions.

---

### 1.3 Replace `boost::lexical_cast` with `std::to_string`
**Files:** `currentprocessid.hpp` (line 68), `textlogger.hpp` (lines 37, 102)

`std::to_string` (C++11) handles every integer-to-string conversion these
files need. No behavior change.

---

### 1.4 Replace `boost::foreach` with range-based `for`
**Files:** `connection.hpp`, `connection_new.hpp`, `datagram.hpp`

`BOOST_FOREACH(x, container)` → `for (auto& x : container)`

---

### 1.5 Replace `boost::array` with `std::array`
**Files:** `connection.hpp`, `connection_new.hpp`, `datagram.hpp`

`boost::array<T, N>` → `std::array<T, N>` (`<array>`, C++11). Identical API.

---

### 1.6 Replace `boost::optional` with `std::optional`
**Files:** `connection.hpp`, `connection_new.hpp`

`boost::optional<T>` → `std::optional<T>` (`<optional>`, C++17). Identical
for the usage patterns present (construction, `operator bool`, `operator*`).

---

## Phase 2 — Memory Management

### 2.1 `interproc/interp.hpp` — remove `std::auto_ptr`
**File:** `commonlibs/interproc/interp.hpp`

`std::auto_ptr` was deprecated in C++11 and **removed in C++17**. The code
will not compile at all under C++17 without this fix.

**Changes:**
- `std::auto_ptr<shared_memory_object>` → `std::unique_ptr<shared_memory_object>`
- `std::auto_ptr<mapped_region>` → `std::unique_ptr<mapped_region>`
- All accesses (`reset()`, `get()`, dereference) remain compatible.

---

### 2.2 `prefix_tree.hpp` — raw pointer array → `std::unique_ptr`
**File:** `commonlibs/prefix_tree.hpp`

The `Vertex` struct owns 26 raw `Vertex*` child pointers and manually
deletes them in the destructor. This is error-prone and violates the Rule
of Zero.

**Changes:**
- `Vertex *edges[26]` → `std::unique_ptr<Vertex> edges[26]`
- Remove explicit destructor (compiler-generated one will correctly call
  `unique_ptr` destructors recursively).
- Replace `new Vertex()` with `std::make_unique<Vertex>()`.
- Replace `edges[i] == NULL` guards with `edges[i] == nullptr` (or just
  `!edges[i]`).
- `edges_refs` is never assigned or used — remove it.

---

### 2.3 `segment_tree.hpp` — raw `new[]`/`delete[]` → `std::vector`
**File:** `commonlibs/segment_tree.hpp`

`M = new int[msize]` with manual `delete [] M` in the destructor can be
replaced with a `std::vector<int>` field, eliminating the manual destructor
and making the class safely copyable/movable.

**Changes:**
- `int *M` field → `std::vector<int> M`
- Constructor: `M = new int[msize]; fill with -1` → `M(msize, -1)` in the
  initialiser list.
- Remove explicit destructor.

---

### 2.4 `singleton.hpp` — rewrite with Magic Statics
**File:** `commonlibs/singleton.hpp`

The file uses `boost::scoped_ptr`, `boost::call_once`, `boost::once_flag`,
and `boost::noncopyable`. All four can be eliminated.

C++11 guarantees that the initialisation of a function-local `static` object
is thread-safe (ISO 6.7 [stmt.dcl]). This "Magic Static" replaces
`call_once` + `once_flag` + `scoped_ptr` with a single line.

**New implementation:**
```cpp
#pragma once
template <class T>
class Singleton {
public:
    static T& instance() {
        static T obj;
        return obj;
    }
    Singleton(const Singleton&)            = delete;
    Singleton& operator=(const Singleton&) = delete;
protected:
    Singleton()  = default;
    ~Singleton() = default;
};
```
Remove all Boost headers. The `= delete` idiom replaces `boost::noncopyable`.

---

## Phase 3 — Threading & Time Utilities

### 3.1 `sleeprelative.hpp` — replace Boost time/thread with `<chrono>`
**File:** `commonlibs/sleeprelative.hpp`

Every Boost symbol in this file has a direct C++11 equivalent.

| Boost | C++11 |
|---|---|
| `boost::get_system_time()` | `std::chrono::system_clock::now()` |
| `boost::posix_time::milliseconds(n)` | `std::chrono::milliseconds(n)` |
| `boost::thread::sleep(tp)` | `std::this_thread::sleep_until(tp)` |
| `boost::system_time` | `std::chrono::system_clock::time_point` |

Also remove the unused `#include <boost/shared_ptr.hpp>`.

---

### 3.2 `posixtime_util.hpp` — replace `boost::posix_time` with `<chrono>`
**File:** `commonlibs/posixtime_util.hpp`

This file converts between POSIX `time_t` / `timeval` structs and
`boost::posix_time::ptime`. Replace `ptime` with
`std::chrono::system_clock::time_point` throughout.

| Boost | C++11 / C++17 |
|---|---|
| `boost::posix_time::ptime` | `std::chrono::system_clock::time_point` |
| `boost::posix_time::from_time_t(t)` | `std::chrono::system_clock::from_time_t(t)` |
| `boost::posix_time::seconds(n)` | `std::chrono::seconds(n)` |
| `boost::posix_time::milliseconds(n)` | `std::chrono::milliseconds(n)` |
| `boost::gregorian::date` | removed (epoch arithmetic done with `duration_cast`) |
| UTC/local conversion | `std::chrono::current_zone()` (C++20) or `std::mktime`/`std::gmtime` |

---

### 3.3 `connection.hpp` & `connection_new.hpp` — threading modernization
**Files:** `commonlibs/connection.hpp`, `commonlibs/connection_new.hpp`

Replace Boost threading primitives; keep Boost.ASIO (no standard equivalent).

| Boost | C++11 |
|---|---|
| `boost::shared_ptr<T>` | `std::shared_ptr<T>` |
| `boost::shared_ptr<boost::thread>(new boost::thread(f))` | `std::make_shared<std::thread>(f)` |
| `boost::bind(&C::m, this, _1)` | `[this](auto arg){ m(arg); }` (lambda) |
| `boost::posix_time::milliseconds(n)` | `std::chrono::milliseconds(n)` (ASIO timer accepts `chrono`) |
| `boost::scoped_ptr<T>` | `std::unique_ptr<T>` |
| `#include <boost/bind.hpp>` | removed |
| `#include <boost/thread.hpp>` | `#include <thread>` |
| `#include <boost/shared_ptr.hpp>` | `#include <memory>` |

`boost::signal` (not `boost::signals2`) has no direct standard equivalent.
Since it is used as a simple callback store, replace with
`std::function<void(...)>` fields and explicit call sites. This is
functionally equivalent for the single-observer pattern used here.

---

### 3.4 `datagram.hpp` — same threading modernization
**File:** `commonlibs/datagram.hpp`

Apply the identical substitutions as §3.3. ASIO socket code is unchanged.

---

## Phase 4 — Filesystem (`textlogger.hpp`)

### 4.1 Replace `boost::filesystem` with `std::filesystem`
**File:** `commonlibs/textlogger.hpp`

The C++17 `<filesystem>` API is intentionally compatible with
`boost::filesystem`. The migration is mostly a find-and-replace.

| Boost | C++17 |
|---|---|
| `boost::filesystem::path` | `std::filesystem::path` |
| `boost::filesystem::exists(p)` | `std::filesystem::exists(p)` |
| `boost::filesystem::is_directory(p)` | `std::filesystem::is_directory(p)` |
| `p / filename` | `p / filename` (identical) |
| `.string()` | `.string()` (identical) |
| `boost::posix_time::ptime` | `std::chrono::system_clock::time_point` |
| `boost::posix_time::second_clock::local_time()` | `std::chrono::system_clock::now()` |
| `boost::posix_time::time_duration` | `std::chrono::duration` |
| `boost::lexical_cast<std::string>(n)` | `std::to_string(n)` |

Also remove the remaining `#include <boost/...>` headers once replaced.

---

## Phase 5 — Consolidate Duplicate Networking Code

### 5.1 Deprecate `connection.hpp` in favour of `connection_new.hpp`
**Files:** `commonlibs/connection.hpp`, `commonlibs/connection_http.hpp`

`connection_new.hpp` is a revised version of `connection.hpp` with improved
error handling and timeout management. Both files are ~650 lines of nearly
identical code. Maintaining both means every bug fix must be applied twice.

**Changes:**
- Migrate `connection_http.hpp` to inherit from the class in `connection_new.hpp`
  instead of `connection.hpp`.
- Add a deprecated stub `connection.hpp` that `#include`s `connection_new.hpp`
  with a `#pragma message("connection.hpp is deprecated, use connection_new.hpp")`.
- In a follow-up, remove `connection.hpp` entirely once no consumer includes it.

---

## Phase 6 — CMake & Build Hygiene

### 6.1 Require C++17 and update Boost components
**File:** `CMakeLists.txt`

- Change `CMAKE_CXX_STANDARD` from `14` to `17`.
- In `find_package(Boost ...)`, list only the components still needed after
  the modernization above: `system` (for ASIO), `thread` (for ASIO internals
  on some platforms), `filesystem` (only if Phase 4 is not complete),
  `interprocess`, `numeric`.
  Remove: `date_time`, `signals`, `foreach`, `lexical_cast`.
- Add `-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT=1` so ASIO uses standard
  executors (forward-compatibility).
- Add AddressSanitizer and UBSan build types for CI:
  ```cmake
  option(ENABLE_SANITIZERS "Build with ASan + UBSan" OFF)
  ```

---

## Boost Dependency Status After Modernization

| Component | Before | After | Reason kept / removed |
|---|---|---|---|
| `boost::asio` | ✓ | ✓ kept | No standard networking TS yet |
| `boost::thread` | ✓ | ✗ removed | `std::thread` (C++11) |
| `boost::bind` | ✓ | ✗ removed | lambdas (C++11) |
| `boost::shared_ptr` / `scoped_ptr` | ✓ | ✗ removed | `std::shared_ptr` / `unique_ptr` |
| `boost::optional` | ✓ | ✗ removed | `std::optional` (C++17) |
| `boost::array` | ✓ | ✗ removed | `std::array` (C++11) |
| `boost::foreach` | ✓ | ✗ removed | range-`for` (C++11) |
| `boost::filesystem` | ✓ | ✗ removed | `std::filesystem` (C++17) |
| `boost::posix_time` / `date_time` | ✓ | ✗ removed | `std::chrono` (C++11) |
| `boost::lexical_cast` | ✓ | ✗ removed | `std::to_string` (C++11) |
| `boost::utility` (`noncopyable`) | ✓ | ✗ removed | `= delete` (C++11) |
| `boost::call_once` / `once_flag` | ✓ | ✗ removed | Magic Statics (C++11) |
| `boost::signal` | ✓ | ✗ removed | `std::function` |
| `boost::interprocess` | ✓ | ✓ kept | No standard IPC equivalent |
| `boost::numeric::ublas` | ✓ | ✓ kept | No standard matrix library |

---

## Implementation Order

The phases are ordered to avoid breaking intermediate states:

1. **Phase 1** (mechanical) — no logic risk, do all files in one commit per step
2. **Phase 2.1** (auto_ptr) — must do before building with `-std=c++17`
3. **Phase 2.2–2.4** (smart pointers / singleton) — independent, can be parallelised
4. **Phase 3** (threading/time) — depends on Phase 1 (array/bind already replaced)
5. **Phase 4** (filesystem) — depends on Phase 3 (posix_time replaced first in posixtime_util)
6. **Phase 5** (consolidation) — do last, after all other changes are stable
7. **Phase 6** (CMake) — update progressively as each Boost component is dropped

Each phase will be a separate commit so the git history is bisect-friendly.
