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

## Terminal vs Popup Preview

Table preview methods such as `head`, `tail`, `view`, `describe`, `aggregate`, and `group`
return a lightweight `DataFrameView`.

```cpp
df.head();          // terminal preview
df.head().show();   // popup if GTK/WebKit is available, else terminal preview
df.describe().show();

auto preview = df.head(10);  // preview is a DataFrameView
preview.show();              // explicit popup/terminal fallback
```

If you need a plain `DataFrame`, disable the deferred preview:

```cpp
pluss::table::DataFrame materialized = df.head(10, false);
```

For the full popup viewer guide, OS support matrix, dependency setup, and `title(...)` naming rules,
see `dataframe_display.md`.

## Typical DataFrame Workflow

```cpp
auto accidents = pluss::table::DataFrame::load_csv(
    "path/to/data.csv",
    {{"nrows", 64}})
    .fillnull(0.0, {"Lifecycle Cost", "Cost Variance (%)"})
    .query("isnotnull(Agency Name)")
    .arrange({"Lifecycle Cost"}, false)
    .head(10, false);

accidents.show();
```

## Convert Between Models

```cpp
auto df = pluss::table::DataFrame::load_csv("path/to/data.csv", {{"nrows", 8}});
auto catalog = df.kindelize();
auto roundtrip_df = catalog.tabelize();

auto registry = df.scribbelize();
auto back_catalog = registry.kindelize();
```

## Save Results

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json", {{"nrows", 12}});

out.save_csv("path/to/output.csv");
out.save_excel("path/to/output.xlsx");
out.save_json("path/to/output.json");
```
