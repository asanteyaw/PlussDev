# Part I: Orientation

PlussMic exists to make C++ data work practical. It gives you table loading,
typed columns, missing-data handling, display, summarization, plotting,
time-series operations, and conversions into more general record models.

## What PlussMic is for

Use PlussMic when you want to:

- load CSV, TXT, Excel, JSON, or Parquet data into C++;
- inspect and summarize a dataset;
- select, rename, reorder, assign, and transform features;
- filter cases with masks or query expressions;
- handle missing numeric, text, and time values;
- group, aggregate, pivot, and correlate data;
- build plotting outputs from table data;
- represent a stack of related tables with `Atlas`;
- convert rectangular data into stream/catalog or registry/slot models;
- save cleaned data back to common file formats.

## The three PlussMic layers

### Table

The table layer is the normal user entry point:

```cpp
#include <pluss/mic.h>

using pluss::table::DataFrame;

auto df = DataFrame::load_csv("sales.csv");
df.info();
df.describe();
```

The table layer has:

| Type | Role |
| --- | --- |
| `Thread` | one typed column |
| `DataFrame` | a 2D table of named `Thread` features |
| `DataFrameView` | a display-oriented frame result returned by preview/summary operations |
| `Atlas` | a stack of related `DataFrame` objects |

### Kindle

Kindle is the stream/catalog model:

| Type | Role |
| --- | --- |
| `Stream` | typed 1D data with optional cases and metadata |
| `Catalog` | named streams |
| `Archive` | stack of catalogs |

Use Kindle when stream metadata and catalog-like organization matter more than
DataFrame ergonomics.

### Bundle

Bundle is the heterogeneous record model:

| Type | Role |
| --- | --- |
| `Slot` | vector of heterogeneous values |
| `Registry` | named slots |
| `Aisle` | collection-oriented bundle layer |

Use Bundle when a field can hold mixed values or when a dataset is closer to a
record registry than a typed rectangular table.

## The core table model

PlussMic uses this table hierarchy:

```text
Atlas -> DataFrame -> Thread -> Series / Note / Timeline / TimeIndex
```

A `DataFrame` owns feature names and feature columns. Each feature is a
`Thread`. A `Thread` wraps one core PlussQit payload kind:

- `Series` for numeric and boolean data;
- `Note` for text data;
- `Timeline` for datetime-like data;
- `TimeIndex` for time-index data.

This design is why a `DataFrame` can contain numeric, text, and time features
without losing type-specific operations.

## Cases and features

PlussMic documentation uses two axis names:

- A **case** is a row-like observation.
- A **feature** is a column-like variable.

Common shape calls:

```cpp
std::int64_t rows = df.ncases();
std::int64_t cols = df.nfeatures();
const auto& names = df.feature_names();
```

Use this language when reading APIs:

- `features(...)` selects or returns columns.
- `cases(...)`, `case_study(...)`, `observe(...)`, `slice(...)`, and `take(...)` operate in row space.
- `set_cases(...)`, `reset_cases(...)`, and `align_cases(...)` manage the case axis.

## Functional style

Most table operations return a new object:

```cpp
auto cleaned = df.fillna(0.0);
auto sorted = cleaned.sort_values({"balance"});
```

Mutating operations use a trailing underscore:

```cpp
df.fillna_(0.0);
df.rename_("old_name", "new_name");
```

Prefer the non-mutating form while learning. Use mutating methods only when you
intentionally want to update an owned object.

## What a beginner should learn first

1. Load a CSV with `DataFrame::load_csv`.
2. Inspect with `head`, `info`, and `describe`.
3. Select features with `features`.
4. Filter cases with `query` or `filter`.
5. Derive features with `assign` or `set_features`.
6. Summarize with `aggregate`, `group_by`, `mean`, `corr`, and `tally`.
7. Save with `save_csv`, `save_excel`, `save_json`, or `save_parquet`.

Everything else in PlussMic builds on those ideas.
