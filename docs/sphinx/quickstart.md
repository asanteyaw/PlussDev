# Quickstart

This quickstart takes you from an empty C++ file to a small Pluss program that
uses arrays, tables, summaries, and serialization.

## 1. Include Pluss

Most user programs start with:

```cpp
#include <pluss/pluss.h>
```

That exports the core `Series`, `Note`, `Timeline`, `TimeIndex`, table, and
plotting surfaces that are available in the built SDK.

## 2. Build or install the SDK

If you already have a staged Pluss SDK, skip to the next section.

From the Pluss repository root:

```bash
cmake -S . -B build
cmake --build build --target pluss_sdk_stage -j
```

The SDK will be staged under:

```text
build/sdk/pluss-sdk-<version>-<os>-<arch>/
```

For more build variants, including Torch-enabled builds, see
[Installation and CMake](installation.md).

## 3. Create a consumer project

Create this `CMakeLists.txt` in your application:

```cmake
cmake_minimum_required(VERSION 3.28)
project(pluss_quickstart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Pluss CONFIG REQUIRED)

add_executable(pluss_quickstart main.cpp)
target_link_libraries(pluss_quickstart PRIVATE Pluss::SDK)
```

Create `main.cpp`:

```cpp
#include <iostream>
#include <pluss/pluss.h>

int main() {
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  const pluss::Series centered = x - x.mean();

  std::cout << "mean: " << x.mean().item<double>() << '\n';
  std::cout << "std: " << x.std(false).item<double>() << '\n';
  std::cout << "centered: " << centered << '\n';

  pluss::save(centered, "centered.plio");
  const auto loaded = pluss::load<pluss::Series>("centered.plio");
  std::cout << "loaded mean: " << loaded.mean().item<double>() << '\n';

  return 0;
}
```

Configure and run:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/pluss-sdk-<version>-<os>-<arch>

cmake --build build
./build/pluss_quickstart
```

Expected behavior:

- The program prints the mean and standard deviation.
- It writes `centered.plio`.
- It loads that file back into a `Series`.

## 4. Load table data

`DataFrame` is the main table type.

```cpp
#include <pluss/pluss.h>

int main() {
  using pluss::table::DataFrame;

  auto df = DataFrame::load_csv("data.csv", {
      {"ncases", 1000},
  });

  df.head(5);
  df.info();
  df.describe();

  auto numeric = df.series_features();
  numeric.mean(true);

  return 0;
}
```

Use typed loader options for non-trivial files:

```cpp
pluss::loader::ReadCsvOptions options;
options.sep = ';';
options.ncases = 1000;
options.usefeats = {"age", "job", "balance"};
options.case_col = "job";

auto df = pluss::table::DataFrame::load_csv("bank-full.csv", options);
```

## 5. Select, filter, derive, and save

```cpp
using pluss::table::DataFrame;

auto df = DataFrame::load_csv("bank-full.csv", {
    {"features", {"age", "job", "balance", "duration"}},
});

auto older = df.query("age >= 45 and job != 'unknown'");

auto enriched = older.assign({{
    "balance_per_second",
    [](const DataFrame& frame) {
      return frame.features("balance") / frame.features("duration");
    },
}});

enriched.group_by(
    {"job"},
    {{"mean_balance", "balance", DataFrame::GroupOp::Mean}},
    true,
    true);

enriched.save_csv("outputs/enriched.csv");
```

The important conventions are:

- `features("name")` returns one column as a `Thread`.
- `features({"a", "b"})` returns a smaller `DataFrame`.
- `query(...)` filters cases with a string expression.
- `assign(...)` derives new features.
- Most transforms return a new object; mutating variants usually end in `_`.

## 6. Know the core types

| Type | Use it for |
| --- | --- |
| `Series` | Numeric and boolean tensor-style data |
| `Note` | Text and byte-vector data |
| `Timeline` | Date/time values |
| `TimeIndex` | 1D time index operations |
| `Thread` | A typed table column over `Series`, `Note`, `Timeline`, or `TimeIndex` |
| `DataFrame` | 2D table of named features over cases |
| `Atlas` | Stack of related `DataFrame` objects |
| `Type` | Runtime union over `Series`, `Note`, `Timeline`, and `TimeIndex` |

## 7. Run repository demos

When developing Pluss itself, repository demos are useful smoke tests:

```bash
cmake --preset cpp_env
cmake --workflow --preset cpp_env_build
```

Common demos include:

```bash
./build/PlussQit/plussqit_codem_series_usage_demo
./build/PlussQit/plussqit_codem_stats_demo
./build/PlussQit/plussqit_codem_autodiff_derivatives_demo
./build/PlussQit/plussqit_coden_note_factory_demo
./build/PlussQit/plussqit_codeh_timeline_dataframe_timeindex_methods_demo
```

Table demos live under `PlussMic/examples/table/`, and plotting demos live under
`PlussPl/examples/`.

## 8. Next pages

Read next:

- [User Guide](user_guide.md) for the conceptual map.
- [Data Workflows](data_workflows.md) for table work.
- [Numerical Workflows](numerical_workflows.md) for `Series` and math.
- [Series Guide](series.md) for exhaustive numeric examples.
- [Loader Guide](loader.md) for I/O options.
