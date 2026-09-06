# Installation and CMake

This page explains how to build Pluss from the repository, stage an SDK, and
consume that SDK from a separate CMake project.

## Requirements

Pluss is a C++20 project. For a normal source build you need:

- CMake 3.21 or newer for repository builds.
- CMake 3.28 or newer for the minimal SDK consumer example.
- A C++20 compiler.
- LibTorch for Torch-enabled `Series` and CodeH paths.
- Optional libzip for Excel I/O.
- Optional Arrow/Parquet dependencies if you build Parquet support.

## Build from the repository

From the repository root:

```bash
cmake --preset cpp_env
cmake --workflow --preset cpp_env_build
```

The plain CMake path also works:

```bash
cmake -S . -B build
cmake --build build -j
```

## Build the SDK target

The SDK is the recommended shape for downstream users.

```bash
cmake -S . -B build
cmake --build build --target pluss -j
cmake --build build --target pluss_sdk_stage -j
```

The staged SDK is written under:

```text
build/sdk/pluss-sdk-<version>-<os>-<arch>/
```

That folder contains public headers, libraries, CMake package files, and docs.

## Build a Torch-enabled SDK

Use a separate build directory for Torch builds:

```bash
cmake -S . -B build-torch-sdk \
  -DPLUSS_ENABLE_TORCH=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo

cmake --build build-torch-sdk --target pluss_sdk_stage -j
```

If Torch is not in a default CMake search location, pass `Torch_DIR`:

```bash
cmake -S . -B build-torch-sdk \
  -DPLUSS_ENABLE_TORCH=ON \
  -DTorch_DIR=/path/to/torch/share/cmake/Torch
```

Keep Torch and non-Torch SDKs in different build directories. The Torch-enabled
SDK exports the compile definitions needed for `pluss::series::from_torch(...)`
and the `Series::tensor()` method when consumers link `Pluss::SDK`.

## Consume Pluss from another project

Minimal `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(pluss_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss CONFIG REQUIRED)

add_executable(pluss_consumer main.cpp)
target_link_libraries(pluss_consumer PRIVATE Pluss::SDK)
```

Minimal `main.cpp`:

```cpp
#include <iostream>
#include <pluss/pluss.h>

int main() {
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << x.mean().item<double>() << '\n';
  return 0;
}
```

Configure the consumer against the staged SDK:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/pluss-sdk-<version>-<os>-<arch>

cmake --build build
./build/pluss_consumer
```

Expected output:

```text
2
```

## Package targets

The installed package exposes:

- `Pluss::SDK`: consumer-friendly umbrella target.
- `Pluss::pluss`: lower-level installed core target.

Most applications should link `Pluss::SDK`.

## Include strategy

Use the umbrella include for applications and tutorials:

```cpp
#include <pluss/pluss.h>
```

Use narrower includes only when building a library that wants to keep compile
dependencies explicit:

```cpp
#include <pluss/core.h>      // Series, Note, Timeline, TimeIndex, Type
#include <pluss/mic.h>       // DataFrame, Thread, Atlas, table I/O
#include <pluss/pl.h>        // plotting
#include <pluss/ma.h>        // math/statistics utilities
#include <pluss/ai.h>        // AI and ML surfaces
#include <pluss/tils.h>      // utility helpers and loader helpers
```

## Common build issues

### CMake cannot find Pluss

Pass the staged SDK folder through `CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/absolute/path/to/pluss-sdk
```

### Torch symbols are missing

Build and consume a Torch-enabled SDK. If your application also uses
`torch::Tensor` directly, keep `find_package(Torch REQUIRED)` in the consumer
project for clarity.

### Excel or Parquet APIs are unavailable

Those features depend on optional dependencies discovered when the SDK is built.
Rebuild the SDK in an environment where the dependency is available, then
reconfigure the consumer project against the new SDK.

### Release stack traces are not useful

Use `RelWithDebInfo` for both SDK and consumer builds. Pluss also keeps
traceback-friendly flags in release-oriented SDK builds when configured with the
project defaults.
