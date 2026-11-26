# Installation and CMake

The PlussII C++ distribution contains headers, compiled libraries, and CMake
package files. Download the archive directly from the [PlussII download page](https://plussible.org/download),
extract it, and point your C++ project at the extracted folder.

## Requirements

You need a C++20 compiler and CMake 3.28 or newer. The download page shows
the available macOS, Linux, and Windows archives by architecture. For a
platform or configuration without an archive, use the source-build steps below.

Some builds enable optional features that need external dependencies such as
LibTorch or Arrow/Parquet. Make those dependencies visible to CMake when
required; the package configuration reports a missing dependency.

## Download a release

Choose your operating system and architecture on the
[PlussII download page](https://plussible.org/download), then download the
matching ZIP. The page gives the checksum and extraction commands for your
selection. The extracted folder contains `include/`, `lib/`, and
`lib/cmake/Pluss/`. Keep that folder in any stable location and use its path
as `CMAKE_PREFIX_PATH`.

## Consume PlussII from another project

Minimal `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(pluss_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss REQUIRED)

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
PLUSSII_DIR="/absolute/path/to/extracted/plussii-cpp-folder"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$PLUSSII_DIR"
cmake --build build --config Release
./build/pluss_consumer
```

The example prints `2`. If CMake cannot locate Pluss, check that the supplied
prefix contains `lib/cmake/Pluss/PlussConfig.cmake`.

On Linux, programs that open PlussView windows (`.show()` on a table, a catalog
or a plot) link against the X11 libraries. Install their development files
first, for example on Ubuntu:

```bash
sudo apt-get install libx11-dev libxi-dev libfreetype-dev
```

Without a display, or without these libraries, `.show()` prints the table as
text in the terminal instead.

## Build from the repository

For a source build you need:

- CMake 3.28 or newer.
- A C++20 compiler.
- LibTorch for optional Torch interop.
- Optional libzip for Excel I/O.
- Optional Arrow/Parquet dependencies if you build Parquet support.

Fetch the public source, configure it, and stage a local distribution folder.
On macOS and Linux, run:

```bash
git clone https://github.com/asanteyaw/PlussDev.git
cd PlussDev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DPLUSSQIT_BUILD_TESTS=OFF -DPLUSSQIT_BUILD_EXAMPLES=OFF
cmake --build build --config Release --target plussii_distribution_stage --parallel 4
```

On Windows, select Windows and “Build from source” on the
[download page](https://plussible.org/download) for the ClangCL and vcpkg
commands.

The result is `build/plussii-cpp/plussii-cpp-<version>-<system>-<architecture>/`. That
folder has the same layout as the extracted binary archive and can be passed
to another project's `CMAKE_PREFIX_PATH`.

The maintainer's Conda preset is available when `cpp_env` is configured:

```bash
cmake --workflow --preset cpp_env_build
```

## Make ZIP and tar archives

The resulting archives are named PlussII C++.

```bash
cmake --build build --target plussii_distribution_archive --parallel 1
```

The archives are written to `build/`. The staged folder remains under `build/plussii-cpp/`.

## Use PlussII with Torch

Torch interop is optional. The PlussII ZIP does not include LibTorch, and a
regular PlussII project does not need it. To call
`pluss::series::from_torch(...)` or `Series::tensor()`, first install a separate
LibTorch distribution for your operating system and architecture. Extract its
ZIP alongside PlussII, then make both folders visible to CMake. The bridge is
compiled from PlussII headers in your application; you do not need to rebuild
the PlussII ZIP with Torch enabled.

Add these lines to your application's `CMakeLists.txt`, after creating `my_app`:

```cmake
find_package(Pluss REQUIRED)
find_package(Torch REQUIRED)
target_link_libraries(my_app PRIVATE Pluss::PlussII ${TORCH_LIBRARIES})
```

With the v0.0.5 archive, include Torch before PlussII in source files that use
the bridge:

```cpp
#include <torch/torch.h>
#include <pluss/pluss.h>
```

Configure with paths to the two extracted folders (the ones containing `lib/`
and `share/`, respectively):

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="/absolute/path/to/plussii-cpp-folder;/absolute/path/to/libtorch"
cmake --build build --config Release
```

LibTorch's [C++ installation guide](https://docs.pytorch.org/cppdocs/installing.html)
explains how to obtain its archive. If
`find_package(Torch REQUIRED)` fails, check that the selected prefix contains
`share/cmake/Torch/TorchConfig.cmake`; `Torch_DIR` may also point directly to
that directory. Define `PLUSS_DISABLE_TORCH_INTEROP` to switch the bridge off.

## Package targets

The installed package exposes:

- `Pluss::PlussII`: full-stack target for new applications.
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
#include <pluss/mic.h>       // DataFrame, Shelf, Atlas, table I/O
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

### CMake cannot find Torch

Install LibTorch separately and add its extracted folder to `CMAKE_PREFIX_PATH`
alongside the PlussII folder, as shown above. A PlussII archive alone cannot
satisfy `find_package(Torch REQUIRED)`.

### Excel or Parquet APIs are unavailable

Those features depend on optional dependencies discovered when the distribution
is built. Rebuild PlussII with the dependency available, then reconfigure your
project against the new distribution.

### Release stack traces are not useful

Use `RelWithDebInfo` for both PlussII and consumer builds. PlussII also keeps
traceback-friendly flags in release builds when configured with project defaults.
