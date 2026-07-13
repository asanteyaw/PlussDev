# User Guide

This guide is the conceptual starting point for Pluss. It explains the main
modules, the core data model, and the workflow style used across the library.

## The ecosystem

Pluss is split into modules, but user applications usually experience it through
one umbrella include and one SDK target.

| Module | Main purpose | Common objects |
| --- | --- | --- |
| PlussQit | Core array, text, time, dispatch, and serialization surface | `Series`, `Note`, `Timeline`, `TimeIndex`, `Type` |
| PlussMic | Tables, loaders, DataFrame operations, Atlas stacks | `Thread`, `DataFrame`, `Atlas` |
| PlussMa | Math, statistics, distributions, finance, optimization | distribution objects, optimizers, time-series helpers |
| PlussPl | Plotting and export | `pluss::pl::Studio`, panes, traces |
| PlussAI | Classical ML and neural/AI surfaces | NodeJ, Lymph, model/workflow types |
| PlussTils | Utility helpers | loader helpers, transforms, view utilities |

Most examples begin with:

```cpp
#include <pluss/pluss.h>
```

Use opt-in module includes such as `<pluss/ai.h>`, `<pluss/ma.h>`, and
`<pluss/tils.h>` when you need surfaces that are intentionally not pulled into
the default umbrella.

## The data model

Pluss uses a layered model:

```text
Atlas -> DataFrame -> Thread -> Series / Note / Timeline / TimeIndex
```

### Series

`Series` is the numeric and boolean tensor-style type. Use it for scalar values,
vectors, matrices, reductions, elementwise math, indexing, random values,
linear algebra, and differentiable calculations.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
auto centered = x - x.mean();
double total = x.sum().item<double>();
```

### Note

`Note` is the vectorized text type. It is used directly for text arrays and
indirectly through text `Thread` columns in a `DataFrame`.

### Timeline and TimeIndex

`Timeline` and `TimeIndex` are for date/time data. Use them for time axes,
calendar extraction, alignment, and time-series workflows.

### Thread

`Thread` is a 1D table column. A `Thread` wraps exactly one core payload kind:
numeric `Series`, text `Note`, `Timeline`, or `TimeIndex`. It also carries
validity/missingness information.

```cpp
pluss::table::Thread values(pluss::make_series({10.0, 12.5, 9.0}));
auto cleaned = values.fillna(0.0).clip(0.0, 100.0);
```

### DataFrame

`DataFrame` is a 2D feature-by-case table. Use it for loaded datasets, EDA,
feature engineering, aggregation, joining, plotting, and export.

```cpp
auto df = pluss::table::DataFrame::load_csv("data.csv");
df.info();
df.describe();

auto numeric = df.series_features();
auto result = numeric.fillna(0.0).mean();
```

### Atlas

`Atlas` is a stack of `DataFrame` objects. Use it when you have repeated frames:
simulation scenarios, dates, experiments, or model slices.

## Functional-by-default style

Most high-level transforms return a new object:

```cpp
auto cleaned = df.fillna(0.0);
auto sorted = cleaned.sort_values({"price"});
```

Mutating methods usually end in `_`:

```cpp
df.fillna_(0.0);
df.rename_("old_name", "new_name");
```

Prefer the non-mutating form in tutorials, pipelines, and notebooks. Use the
mutating form in performance-sensitive code where you intentionally update an
owned object.

## Cases and features

Pluss table docs use two axis names consistently:

- A **case** is a row-like observation.
- A **feature** is a column-like variable.

Common operations:

```cpp
std::int64_t rows = df.ncases();
std::int64_t cols = df.nfeatures();

auto names = df.features();
auto price = df.features("price");
auto subset = df.features({"price", "volume"});
auto first_rows = df.head(10);
```

## Loading and saving

Use `DataFrame` convenience loaders for table data:

```cpp
auto csv = pluss::table::DataFrame::load_csv("data.csv");
auto xlsx = pluss::table::DataFrame::load_excel("report.xlsx", "Prices");
auto json = pluss::table::DataFrame::load_json("records.json");
```

Use `pluss::save` and `pluss::load<T>` for core object serialization:

```cpp
auto x = pluss::randn({3, 4});
pluss::save(x, "x.plio");
auto restored = pluss::load<pluss::Series>("x.plio");
```

See [Loader Guide](loader.md) and [I/O Save/Load Guide](io_save_load_guide.md)
for detailed options.

## Analysis workflow

A typical Pluss analysis has this shape:

```cpp
using pluss::table::DataFrame;

auto raw = DataFrame::load_csv("data/sales.csv", {
    {"ncases", 10000},
    {"features", {"region", "price", "units", "date"}}
});

auto cleaned = raw
    .fillna(0.0, {"price", "units"})
    .assign({{
        "revenue",
        [](const DataFrame& frame) {
          return frame.features("price") * frame.features("units");
        },
    }});

cleaned.group_by(
    {"region"},
    {{"avg_revenue", "revenue", DataFrame::GroupOp::Mean}},
    true,
    true);

cleaned.save_csv("outputs/sales_clean.csv");
```

## Numeric workflow

Use `Series` for array-style operations:

```cpp
auto x = pluss::linspace(0.0, 1.0, 11);
auto y = x.pow(2.0) + 1.0;
auto z = y.mean();
```

For distribution and statistics work, include PlussMa:

```cpp
#include <pluss/ma.h>

pluss::subsp::stats::distr::Normal normal{0.0, 1.0};
auto xs = pluss::make_series({-1.0, 0.0, 1.0});
auto logp = normal.logpdf(xs);
```

## Plotting workflow

For table-level plots, start from `DataFrame::plot()` when available for the
shape of data you are using. For explicit plot construction, use PlussPl:

```cpp
#include <pluss/pl.h>

namespace pl = pluss::pl;

auto fig = pl::studio("trend");
fig.pane2("Signal")
    .trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 4, 9})
    .title("x^2");
fig.save_svg("trend.svg");
```

## Choosing the right docs page

| Task | Start here |
| --- | --- |
| Install or consume Pluss | [Installation and CMake](installation.md) |
| Learn by building a first program | [Quickstart](quickstart.md) |
| Load CSV, Excel, JSON, Parquet | [Loader Guide](loader.md) |
| Work with tables | [Data Workflows](data_workflows.md) |
| Work with numeric arrays | [Series Guide](series.md) |
| Query/filter tables | [Series Query User Guide](series_query_user_guide.md) |
| Save/load core objects | [I/O Save/Load Guide](io_save_load_guide.md) |
| Plot manually | [PlussPl User Guide](plusspl_user_guide.md) |
| Use distributions | [Distributions User Guide](plussma_distributions_user_guide.md) |
| Optimize functions | [Optimization User Guide](plussma_optimization_user_guide.md) |
| Build ML workflows | [Lymph User Guide](lymph_user_guide.md), [NodeJ User Guide](nodej_user_guide.md) |

## API reference strategy

The API reference pages are intentionally detailed and method-oriented. Read
the guides first, then use these pages when you need exact method names:

- [Series API Reference](series_api_reference.md)
- [Series All Methods Usage](series_all_methods_usage.md)
- [Note API Reference](note_api_reference.md)
- [Timeline API Reference](timeline_api_reference.md)
- [TimeIndex API Reference](timeindex_api_reference.md)
- [PlussPl API Reference](plusspl_api_reference.md)
