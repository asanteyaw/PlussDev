# Pluss SDK Install Guide

`pluss` can now be consumed as an installed CMake package in the same style as LibTorch:

1. build the SDK,
2. stage or archive it,
3. point your downstream project at the SDK folder,
4. use `find_package(Pluss CONFIG REQUIRED)`.

## Build The SDK

From the `PlussDev` repo root:

```bash
cmake -S . -B build
cmake --build build --target pluss -j
```

Default behavior (new build directories):

- `CMAKE_BUILD_TYPE` defaults to `RelWithDebInfo` on single-config generators.
- `PLUSS_TRACE_FRIENDLY_RELEASE=ON` keeps frame pointers and minimal line tables in `Release` builds for better stack traces.
- This is meant to give meaningful errors without requiring users to manually switch to full debug workflows.

## Build A Torch-Enabled SDK

If you want `pluss` <-> LibTorch interop (`PLUSS_ENABLE_TORCH=ON`), configure a separate build tree for Torch:

```bash
rm -rf build-torch-sdk

cmake -S . -B build-torch-sdk \
  -DPLUSS_ENABLE_TORCH=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPython3_EXECUTABLE="$CONDA_PREFIX/bin/python3" \
  -DPKG_CONFIG_EXECUTABLE="$CONDA_PREFIX/bin/pkg-config"
  
cmake --build build-torch-sdk --target pluss_sdk_stage --parallel "$(sysctl -n hw.logicalcpu)"
```

Torch-enabled staged SDK path:

```text
build-torch-sdk/sdk/pluss-sdk-<version>-<os>-<arch>/
```

Notes:

- Build Torch and non-Torch SDKs in separate build directories (for example `build/` and `build-torch-sdk/`).
- For strongest file:line call traces in downstream apps, build downstream in `RelWithDebInfo` too (or at least with symbols).
- Ensure your env is activated first (so `CONDA_PREFIX` is exported). Example: `conda activate cpp_env`.
- In multiline shell commands, ensure `\` is the last character on the line (no trailing spaces), otherwise zsh may execute `-D...` lines as separate commands.
- On macOS, CMake now auto-selects `cpp_env/bin/clang` and `cpp_env/bin/clang++` for compile when compilers are not explicitly pinned.
- If your build directory was configured earlier with a different compiler, delete it (or pass `-DCMAKE_C_COMPILER=... -DCMAKE_CXX_COMPILER=...`) so CMake re-detects the toolchain.
- Link stage may still use `/usr/bin/clang++` internally when Conda’s linker wrapper is detected, to preserve framework-link correctness.
- `PLUSS_ENABLE_TORCH=ON` now auto-detects `Torch_DIR` from the active env (`CONDA_PREFIX` / `Python3_EXECUTABLE`) when possible.
- URL loader support (`load_csv/load_txt/load_excel` on `http(s)`) now wires curl automatically:
  - uses system curl when available;
  - falls back to CMake FetchContent curl when needed (`PLUSSMIC_FETCH_CURL=ON`, default);
  - staged SDK export now carries a curl link artifact, so downstream projects do not need to install curl in their env separately.
- If Torch is installed in a non-default location, pass:
  - `-DTorch_DIR=/path/to/torch/share/cmake/Torch`
  - optionally `-DCMAKE_PREFIX_PATH="/extra/prefix;/another/prefix"`
- In a Torch-enabled SDK build, `PLUSS_ENABLE_TORCH` is exported by CMake targets. Consumers linking `Pluss::SDK`/`PlussQit::CodeM` automatically get `pluss::from_torch(...)` and `pluss::to_torch(...)` from `<pluss/pluss.h>` / `<pluss/core.h>`.
- Unified core serialization is available via `pluss::io::save/load` and top-level aliases `pluss::save/load` for `Series/Note/Timeline/TimeIndex` and supported STL containers. See `docs/IO_SAVE_LOAD_GUIDE.md`.
- If your app also uses `torch::Tensor` directly, keep `find_package(Torch REQUIRED)` in your consumer CMake for clarity.

## Produce A Downloadable SDK Folder

To create a self-contained SDK folder:

```bash
cmake --build build --target pluss_sdk_stage --parallel "$(sysctl -n hw.logicalcpu)"
```

That stages the SDK at:

```text
build/sdk/pluss-sdk-<version>-<os>-<arch>/
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
cmake --build build --target pluss_sdk_archive --parallel "$(sysctl -n hw.logicalcpu)"
```

That emits `.zip` and `.tar.gz` archives in the build directory.

## Consume The SDK In Another Project

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
target_link_libraries(pluss_consumer PRIVATE Pluss::SDK)
```

Minimal `main.cpp`:

```cpp
#include <pluss/pluss.h>
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

Configure the consumer project against the staged SDK folder:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/pluss-sdk-<version>-<os>-<arch>
cmake --build build
./build/pluss_consumer
```

If your staged SDK includes PlussMic parquet support (Arrow/Parquet found during SDK build), include the Arrow/Parquet prefix too:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/path/to/pluss-sdk-<version>-<os>-<arch>;/Users/yawasante/software/miniconda3/envs/cpp_env"
```

## Package Targets

- `Pluss::pluss`: installed core static library target
- `Pluss::SDK`: consumer-friendly top-level alias for the SDK entrypoint

## Notes

- `pluss` currently ships as a source-built SDK folder plus archives, not yet via Conda.
- The installed SDK intentionally carries only the public header trees needed by the exported API, rather than mirroring the entire monorepo.
