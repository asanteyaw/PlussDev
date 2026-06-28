# Part II: Installation, Includes, and Build Setup

PlussMic is usually consumed through the Pluss SDK. In a downstream project,
link `Pluss::SDK` and include either the Pluss umbrella header or the PlussMic
module header.

## Recommended include

For applications:

```cpp
#include <pluss/pluss.h>
```

For code that only wants PlussMic:

```cpp
#include <pluss/mic.h>
```

`<pluss/mic.h>` includes table, loader, Kindle, and Bundle surfaces:

```cpp
#include "pluss/bundle/bundle.h"
#include "pluss/kindle/kindle.h"
#include "pluss/loader/loader.h"
#include "pluss/table/table.h"
```

It also includes PlussPl when available, which enables table plotting.

## Minimal consumer CMake

```cmake
cmake_minimum_required(VERSION 3.28)
project(plussmic_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss CONFIG REQUIRED)

add_executable(plussmic_app main.cpp)
target_link_libraries(plussmic_app PRIVATE Pluss::SDK)
```

Configure against a staged SDK:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/pluss-sdk
cmake --build build
```

## Minimal program

```cpp
#include <iostream>
#include <pluss/mic.h>

int main() {
  auto df = pluss::table::DataFrame({
      {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
      {"revenue", pluss::make_series({120.0, 95.0, 140.0})},
  });

  df.head();
  df.describe();
  return 0;
}
```

## Optional dependencies

PlussMic can expose functionality conditionally based on how the SDK was built.

| Feature | Dependency |
| --- | --- |
| Excel loading/saving | Excel support in the PlussMic build, commonly via libzip-backed paths |
| Parquet loading/saving | Arrow/Parquet dependencies discovered during SDK build |
| Plot exports | PlussPl availability |
| Popup display | GTK/WebKit or supported platform display stack |
| URL loading | curl support in the SDK build |

If an optional format is missing, rebuild the SDK in an environment where that
dependency is available, then reconfigure your consumer project.

## Development build inside this repository

From the repository root:

```bash
cmake --preset cpp_env
cmake --workflow --preset cpp_env_build
```

Useful PlussMic examples live under:

```text
PlussMic/examples/table/
PlussMic/examples/kindle/
PlussMic/examples/bundle/
```

The examples are a good way to verify that a build has working table I/O,
display, plotting, and model conversion.

## Header choices

Use broad headers while learning:

```cpp
#include <pluss/pluss.h>
```

Use module headers for production libraries with tighter dependencies:

```cpp
#include <pluss/mic.h>
#include <pluss/table/table.h>
#include <pluss/loader/loader.h>
#include <pluss/kindle/kindle.h>
#include <pluss/bundle/bundle.h>
```

Use source-local headers only when working inside the Pluss repository.
