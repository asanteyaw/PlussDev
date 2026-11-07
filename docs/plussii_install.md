# PlussII C++ Distribution

For published versions, select your operating system and download the matching
ZIP from the [PlussII download page](https://plussible.org/download).
Extract it and set `CMAKE_PREFIX_PATH` to the extracted folder. Each archive
contains public headers, compiled libraries, and CMake package files.

This document also describes how maintainers build the distribution from source.
The source build flow is:

1. build PlussII,
2. stage or archive the distribution,
3. point a downstream project at the extracted folder,
4. use `find_package(Pluss CONFIG REQUIRED)`.

## Build The Distribution From Source

Install CMake 3.28 or newer, a C++20 compiler, and Python 3.11 or newer.
From the `PlussDev` repo root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DPLUSSQIT_BUILD_TESTS=OFF -DPLUSSQIT_BUILD_EXAMPLES=OFF
cmake --build build --config Release --target plussii_distribution_stage --parallel 4
```

On Windows x64, use Visual Studio 2022 with its Clang tools and set
`VCPKG_ROOT` to your vcpkg installation. Install the static SQLite dependency,
then configure with the matching toolset and triplet in PowerShell:

```powershell
& "$env:VCPKG_ROOT\vcpkg.exe" install sqlite3:x64-windows-static
cmake -S . -B build -G "Visual Studio 17 2022" -T ClangCL `
  -DBUILD_TESTING=OFF -DPLUSSQIT_BUILD_TESTS=OFF -DPLUSSQIT_BUILD_EXAMPLES=OFF `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release --target plussii_distribution_stage --parallel 4
```

Default behavior (new build directories):

- `CMAKE_BUILD_TYPE` defaults to `RelWithDebInfo` on single-config generators.
- `PLUSS_TRACE_FRIENDLY_RELEASE=ON` keeps frame pointers and minimal line tables in `Release` builds for better stack traces.
- This is meant to give meaningful errors without requiring users to manually switch to full debug workflows.

### Maintainer preset

From the root folder run:

```bash
cmake --workflow --preset cpp_env_build
```

## Build A Torch-Enabled Distribution

If you want `pluss` <-> LibTorch interop (`PLUSS_ENABLE_TORCH=ON`), configure a separate build tree for Torch:


```bash
cmake -S . -B build-torch \
  -DPLUSS_ENABLE_TORCH=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPython3_EXECUTABLE="$CONDA_PREFIX/bin/python3" \
  -DPKG_CONFIG_EXECUTABLE="$CONDA_PREFIX/bin/pkg-config"

cmake --build build-torch --target plussii_distribution_stage --parallel "$(sysctl -n hw.logicalcpu)"
```



Torch-enabled staged distribution path:

```text
build-torch/plussii-cpp/plussii-cpp-<version>-<os>-<arch>/
```

Notes:

- Build Torch and non-Torch distributions in separate build directories (for example `build/` and `build-torch/`).
- For strongest file:line call traces in downstream apps, build downstream in `RelWithDebInfo` too (or at least with symbols).
- Ensure your env is activated first (so `CONDA_PREFIX` is exported). Example: `conda activate cpp_env`.
- In multiline shell commands, ensure `\` is the last character on the line (no trailing spaces), otherwise zsh may execute `-D...` lines as separate commands.
- On macOS, CMake defaults to Apple Clang so the compiler and resource headers match the installed Xcode SDK. Explicit CMake compiler settings and `CC`/`CXX` environment choices take priority.
- If your build directory was configured earlier with a different compiler, delete it (or pass `-DCMAKE_C_COMPILER=... -DCMAKE_CXX_COMPILER=...`) so CMake re-detects the toolchain.
- Explicit Conda Clang builds use `/usr/bin/clang++` for executable linking, including CMake compiler and Threads probes. The Conda compiler must still support the installed SDK headers.
- `PLUSS_ENABLE_TORCH=ON` now auto-detects `Torch_DIR` from the active env (`CONDA_PREFIX` / `Python3_EXECUTABLE`) when possible.
- URL loader support (`load_csv/load_txt/load_excel` on `http(s)`) now wires curl automatically:
  - uses system curl when available;
  - falls back to CMake FetchContent curl when needed (`PLUSSMIC_FETCH_CURL=ON`, default);
  - the staged distribution carries a curl link artifact when needed.
- If Torch is installed in a non-default location, pass:
  - `-DTorch_DIR=/path/to/torch/share/cmake/Torch`
  - optionally `-DCMAKE_PREFIX_PATH="/extra/prefix;/another/prefix"`
- In a Torch-enabled build, `PLUSS_ENABLE_TORCH` is exported by CMake targets. Consumers linking `Pluss::PlussII`/`PlussQit::CodeM` get `pluss::series::from_torch(...)` and `Series::tensor()` from `<pluss/pluss.h>` / `<pluss/core.h>`.
- `Pluss::PlussII` propagates traceback-friendly compile flags to consumers (`-fno-omit-frame-pointer` and minimal line tables in Release).
- Unified core serialization is available via `pluss::io::save/load` and top-level aliases `pluss::save/load` for `Series/Note/Timeline/TimeIndex` and supported STL containers. See `docs/IO_SAVE_LOAD_GUIDE.md`.
- If your app also uses `torch::Tensor` directly, keep `find_package(Torch REQUIRED)` in your consumer CMake for clarity.

## Produce A Downloadable Distribution Folder

To stage the distribution:

```bash
cmake --build build --config Release --target plussii_distribution_stage --parallel 4
```

That stages PlussII at:

```text
build/plussii-cpp/plussii-cpp-<version>-<os>-<arch>/
```

The staged folder contains:

- `include/`
- `lib/`
- `share/doc/`
- `lib/cmake/Pluss/`
- `lib/cmake/PlussQit/`

## Produce ZIP/TGZ Archives

To create distributable archives:

```bash
cmake --build build --target plussii_distribution_archive --parallel "$(sysctl -n hw.logicalcpu)"
```

That emits `.zip` and `.tar.gz` archives in the build directory.

## Consume PlussII In Another Project

Minimal consumer `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(pluss_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

find_package(Pluss CONFIG REQUIRED)

add_executable(pluss_consumer main.cpp)
target_link_libraries(pluss_consumer PRIVATE Pluss::PlussII)
```

Minimal `main.cpp`:

```cpp
#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <iostream>

int main() {
  pluss::Series x = pluss::make_series({0.5});
  pluss::subsp::stats::distr::Normal dist{0.0, 1.0};
  std::cout << dist.logpdf(x).item<double>() << '\n';
  return 0;
}
```

Host extraction quick reference:

```cpp
pluss::Series s0 = pluss::make_series(1.0);
double x = s0.host<double>();  // scalar

pluss::Series s1 = pluss::make_series({1.0, 2.0, 3.0});
std::vector<double> v = s1.host<double>();  // 1-D

pluss::Series s2 = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
auto a = s2.host<double>().array(); // N-D flat data + shape/stride metadata

auto meta = s2.attr();              // metadata + host values bundle
std::cout << meta << '\n';          // printable summary
std::cout << meta.dtype << '\n';    // direct field access
std::cout << meta.is_contiguous() << '\n';
std::cout << meta.rank() << '\n';
std::cout << meta.values << '\n';   // host payload preview
```

Set `PLUSSII_DIR` to the extracted distribution folder, then configure:

```bash
PLUSSII_DIR="/absolute/path/to/plussii-cpp-0.1.1-Darwin-arm64"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$PLUSSII_DIR"
cmake --build build
./build/pluss_consumer
```

If the distribution includes PlussMic parquet support, include the Arrow/Parquet prefix too:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$PLUSSII_DIR;/path/to/dependencies"
```

## Package Targets

- `Pluss::pluss`: installed core static library target
- `Pluss::PlussII`: full stack target for applications

## Notes

- Release ZIPs for macOS arm64, Linux x64, and Windows x64 are published together
  after all three archive and consumer checks pass.
- The distribution carries the public header trees needed by the exported API.
