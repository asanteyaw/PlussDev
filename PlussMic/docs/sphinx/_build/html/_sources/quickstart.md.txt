# Quickstart

## Include Paths

```cpp
#include <pluss/pluss.h>
// or, when you only want PlussMic:
#include <pluss/mic.h>
```

## Load A Table Fast

```cpp
auto df = pluss::table::DataFrame::load_csv(
    "path/to/data.csv",
    {{"nrows", 16}});

std::cout << df << "\n";
df.info();
df.describe();
```

## Start With A Small In-Memory Frame

```cpp
auto sales = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"revenue", pluss::table::Thread(pluss::make_series({120.0, 95.0, 140.0}))},
    {"opened_at", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:00:00Z",
        "2024-01-02T09:00:00Z",
        "2024-01-03T09:00:00Z",
    }))},
});

sales.head();
```

## Typical DataFrame Workflow

```cpp
auto accidents = pluss::table::DataFrame::load_csv(
    "path/to/data.csv",
    {{"nrows", 64}})
    .fillna(0.0, {"Lifecycle Cost", "Cost Variance (%)"})
    .query("isnotnull(Agency Name)")
    .sort_values({"Lifecycle Cost"}, false)
    .head(10, false);

accidents.show();
```

## Convert Between Models

```cpp
auto df = pluss::table::DataFrame::load_csv("path/to/data.csv", {{"nrows", 8}});
auto catalog = df.kindelize();
auto roundtrip_df = catalog.tabelize();

auto registry = pluss::bundle::Registry::from_kindle(catalog);
auto back_catalog = registry.to_kindle();
```

## Save Results

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json", {{"nrows", 12}});

out.save_csv("path/to/output.csv");
out.save_excel("path/to/output.xlsx");
out.save_json("path/to/output.json");
```
