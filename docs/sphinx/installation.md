# Installation and CMake

The PlussII C++ distribution contains headers, compiled libraries, and CMake
package files. Download a release archive, extract it, and point your C++
project at the extracted folder.

## Requirements

You need a C++20 compiler and CMake 3.28 or newer. Download the archive for
your operating system and architecture. Currently the release workflow builds
macOS Apple Silicon archives; build from source on other systems.

Some builds enable optional features that need external dependencies such as
LibTorch or Arrow/Parquet. Make those dependencies visible to CMake when
required; the package configuration reports a missing dependency.

## Download a release

For published versions, download an archive from the
[PlussII releases page](https://github.com/asanteyaw/PlussDev/releases). Choose
the `plussii-cpp-<version>-<os>-<arch>.zip` file matching your machine, then
extract it. The folder inside contains `include/`, `lib/`, and
`lib/cmake/Pluss/`.

The release also provides `SHA256SUMS.txt`. Set `PLUSSII_ARCHIVE` to the file
you downloaded and compare its checksum with the entry in that file:

```bash
PLUSSII_ARCHIVE="plussii-cpp-0.1.0-Darwin-arm64.zip"
shasum -a 256 "$PLUSSII_ARCHIVE"
```

The release page is the download source. Before the first release, or for a
different configuration, build the distribution from the repository.

## Consume PlussII from another project

Minimal `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(pluss_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss CONFIG REQUIRED)

add_executable(pluss_consumer main.cpp)
target_link_libraries(pluss_consumer PRIVATE Pluss::PlussII)
```

Minimal `main.cpp`:

```cpp
#include <iostream>
#include <pluss/pluss.h>

int main() {
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << x.mean().item<double>() << '\n';
}
```

Set `PLUSSII_DIR` to the extracted distribution folder, then configure:

```bash
PLUSSII_DIR="/absolute/path/to/plussii-cpp-0.1.0-Darwin-arm64"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$PLUSSII_DIR"
cmake --build build
./build/pluss_consumer
```

The example prints `2`. If CMake cannot locate Pluss, check that the supplied
prefix contains `lib/cmake/Pluss/PlussConfig.cmake`.

## Build from the repository

For a source build you need:

- CMake 3.28 or newer.
- A C++20 compiler.
- LibTorch for optional Torch interop.
- Optional libzip for Excel I/O.
- Optional Arrow/Parquet dependencies if you build Parquet support.

From the repository root:

```bash
cmake -S . -B build
cmake --build build -j
```

The maintainer's Conda preset is available when `cpp_env` is configured:

```bash
cmake --workflow --preset cpp_env_build
```

## Create a local distribution

The existing `pluss_sdk_stage` and `pluss_sdk_archive` target names remain for
build script compatibility. The resulting archive is named PlussII C++.

```bash
cmake -S . -B build
cmake --build build --target pluss_sdk_stage -j
cmake --build build --target pluss_sdk_archive -j1
```

The staged distribution is written under:

```text
build/sdk/plussii-cpp-<version>-<os>-<arch>/
```

That folder contains public headers, libraries, CMake package files, and docs.

## Build a Torch-enabled distribution

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

Keep Torch and non-Torch distributions in different build directories. The
Torch-enabled distribution exports the compile definitions needed for
`pluss::series::from_torch(...)` and `Series::tensor()`.

## Package targets

The installed package exposes:

- `Pluss::PlussII`: full-stack target for new applications.
- `Pluss::SDK`: compatibility target for existing applications.
- `Pluss::pluss`: lower-level installed core target.

Applications should link `Pluss::PlussII`.

## Include strategy

Use the umbrella include for applications and tutorials:

```cpp
#include <pluss/pluss.h>
```

Use narrower includes only when building a library that wants to keep compile
dependencies explicit:

```cpp
#include <pluss/core.h>      // Series, Note, Timeline, TimeIndex, Type
#include <pluss/mic.h>       // DataFrame, Slate, Atlas, table I/O
#include <pluss/pl.h>        // plotting
#include <pluss/ma.h>        // math/statistics utilities
#include <pluss/ai.h>        // AI and ML surfaces
#include <pluss/tils.h>      // utility helpers and loader helpers
```

## Common build issues

### CMake cannot find Pluss

Pass the extracted distribution folder through `CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$PLUSSII_DIR"
```

### Torch symbols are missing

Build and consume a Torch-enabled distribution. If your application also uses
`torch::Tensor` directly, keep `find_package(Torch REQUIRED)` in the consumer
project for clarity.

### Excel or Parquet APIs are unavailable

Those features depend on optional dependencies discovered when the distribution
is built. Rebuild PlussII with the dependency available, then reconfigure your
project against the new distribution.

### Release stack traces are not useful

Use `RelWithDebInfo` for both PlussII and consumer builds. PlussII also keeps
traceback-friendly flags in release builds when configured with project defaults.
