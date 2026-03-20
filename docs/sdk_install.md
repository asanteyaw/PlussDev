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

## Build A Torch-Enabled SDK

If you want `pluss` <-> LibTorch interop (`PLUSS_ENABLE_TORCH=ON`), configure a separate build tree for Torch:

```bash
cmake -S . -B build-torch-sdk \
  -DPLUSS_ENABLE_TORCH=ON \
  -DOPENSSL_ROOT_DIR=/Users/yawasante/software/miniconda3/envs/cpp_env \
  -DCMAKE_PREFIX_PATH=/Users/yawasante/software/miniconda3/envs/cpp_env/lib/python3.12/site-packages/torch \
  -DTorch_DIR=/Users/yawasante/software/miniconda3/envs/cpp_env/lib/python3.12/site-packages/torch/share/cmake/Torch \
  -DPKG_CONFIG_EXECUTABLE=/Users/yawasante/software/miniconda3/envs/cpp_env/bin/pkg-config

cmake --build build-torch-sdk --target pluss_sdk_stage -j1
```

Torch-enabled staged SDK path:

```text
build-torch-sdk/sdk/pluss-sdk-<version>-<os>-<arch>/
```

Notes:

- Build Torch and non-Torch SDKs in separate build directories (for example `build/` and `build-torch-sdk/`).
- If your app also uses `torch::Tensor` directly, keep `find_package(Torch REQUIRED)` in your consumer CMake for clarity.

## Produce A Downloadable SDK Folder

To create a self-contained SDK folder:

```bash
cmake --build build --target pluss_sdk_stage
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
cmake --build build --target pluss_sdk_archive
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

Configure the consumer project against the staged SDK folder:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/pluss-sdk-<version>-<os>-<arch>
cmake --build build
./build/pluss_consumer
```

## Package Targets

- `Pluss::pluss`: installed core static library target
- `Pluss::SDK`: consumer-friendly top-level alias for the SDK entrypoint

## Notes

- `pluss` currently ships as a source-built SDK folder plus archives, not yet via Conda.
- The installed SDK intentionally carries only the public header trees needed by the exported API, rather than mirroring the entire monorepo.
