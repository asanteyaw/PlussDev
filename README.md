# PlussMa
Next-generation technical computing and AI stack for C++.

## Quick Start

```cpp
#include <pluss/pluss.h>

int main() {
  using pluss::table::DataFrame;

  auto df = DataFrame::load_csv("../../PlussMic/data/messy_data.csv");
  df.head();
  df.info();

  auto by_agency = df.tally("Agency Name", 10);
  auto numeric = df.series();

  auto df2 = df.set_features("Lifecycle Cost x2", df.features("Lifecycle Cost") * 2.0);
  df2.save_csv("messy_with_derived.csv");
}
```

## Core Types

- `pluss::Series` (numeric/bool 1D)
- `pluss::Note` (text 1D)
- `pluss::Timeline` / `pluss::TimeIndex` (time 1D)
- `pluss::table::Thread` (1D table abstraction over pluss types)
- `pluss::table::DataFrame` (2D features x cases)
- `pluss::table::Atlas` (3D stack of DataFrames)

Doctrine:

- `Atlas -> DataFrame -> Thread -> (Series/Note/Timeline/TimeIndex)`

## DataFrame API Style

- Feature axis terminology: `features(...)`
- Case axis terminology: `cases(...)`
- Preferred methods: `sort_cases`, `tally`, `group_by`, `set_features`

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
