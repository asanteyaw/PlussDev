# Plusses
Next-generation technical computing and AI Ecosystem for C++.

## Quick Start

```cpp
#include <pluss/pluss.h>

int main() {
  using pluss::table::DataFrame;

  auto df = DataFrame::load_csv("path/to/data.csv");
  df.head();
  df.info();

  auto by_agency = df.features("Agency Name").freq();
  auto numeric_data = df.feataxis().series();

  auto df2 = df.assign("Lifecycle Cost x2", df.features("Lifecycle Cost") * 2.0);
  df2.save_csv("path/to/save/data.csv");
}
```

## Core Types

- `pluss::Series` (numeric/bool ND)
- `pluss::Note` (text ND)
- `pluss::Timeline` (time ND)
- `pluss::TimeIndex` (time 1D)
- `pluss::Duration` (elapsed time ND)

Doctrine:

- `Atlas -> DataFrame -> Thread -> (Series/Note/Timeline/TimeIndex)`

## DataFrame API Style

- Feature axis terminology: `features(...)`
- Case axis terminology: `cases(...)`
- Preferred methods: `transform`, `arrange`, `assign`, `set`

## Loading and Saving

```cpp
using pluss::table::DataFrame;

auto csv = DataFrame::load_csv("data.csv");                // auto separator + auto engine
auto xlsx = DataFrame::load_excel("report.xlsx", "Prices");

csv.save_csv("out.csv");                                   // add_cases=false by default
xlsx.save_excel("out.xlsx", "Sheet1");
```

Workbook mode for Excel:

```cpp
auto wb = DataFrame::load_excel("report.xlsx", nullptr, true);
for (const auto& name : wb.sheet_names()) {
  wb.sheets(name).head();
}
```

## Documentation

- `docs/PLUSS_FOUNDATIONS.md`
- `docs/DATAFRAME_ATLAS_USER_GUIDE.md`
- `docs/THREAD_API_GUIDE.md`
- `PlussMic/examples/table/`

## Build

### Preferred (preset)

```bash
cmake --preset cpp_env
cmake --workflow --preset cpp_env_build
```

### Manual

```bash
cmake -S . -B build
cmake --build build -j
```

## Requirements

- C++20 compiler
- CMake 3.21+
- LibTorch (for CodeM/CodeH paths)
- Optional: libzip for Excel I/O

## License

- `LICENSE`
- `NOTICE.md`
