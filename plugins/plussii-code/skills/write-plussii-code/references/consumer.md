# C++ consumer setup

The following is based on `docs/sphinx/installation.md` in PlussDev. Recheck that guide for the user's release before using it.

The extracted PlussII distribution has `include/`, `lib/`, and `lib/cmake/Pluss/`. It requires a C++20 compiler and CMake 3.28 or newer.

For a new project containing `CMakeLists.txt` and `main.cpp`:

```cmake
cmake_minimum_required(VERSION 3.28)
project(pluss_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss REQUIRED)

add_executable(pluss_example main.cpp)
target_link_libraries(pluss_example PRIVATE Pluss::PlussII)
```

```cpp
#include <iostream>
#include <pluss/pluss.h>

int main() {
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << x.mean().item<double>() << '\n';
}
```

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/absolute/path/to/extracted/plussii-cpp-folder"
cmake --build build --config Release
./build/pluss_example
```

The expected output is `2`. On Windows, the executable is usually `build/Release/pluss_example.exe` with a multi-configuration generator. Adapt paths, target names, and generator commands to the user's project.
