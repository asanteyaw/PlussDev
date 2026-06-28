# DataFrame Guide

## What DataFrame Is Good At

`pluss::table::DataFrame` is the main table-analysis surface in PlussMic.
Use it when you want:

- named features with mixed typed columns
- a case axis that can be numeric, note-based, timeline-based, or time-index based
- table verbs such as `query`, `assign`, `group_by`, `pivot_table`, `join`, `fillna`, and `describe`
- import/export across CSV, TXT, JSON, Excel, and Parquet

## Construction And Loading

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"opened", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T08:00:00Z",
        "2024-01-02T08:00:00Z",
        "2024-01-03T08:00:00Z",
    }))},
});

auto csv_df = pluss::table::DataFrame::load_csv(
    "path/to/data.csv",
    {{"nrows", 32}});

auto excel_df = pluss::table::DataFrame::load_excel(
    "path/to/data.xlsx",
    {{"sheetindex", 0}, {"header", true}});

auto json_df = pluss::table::DataFrame::load_json(
    "path/to/data.json",
    {{"nrows", 24}});
```

## Inspect Rows, Features, And Cases

```cpp
df.head();
df.tail();
df.info();
df.describe();

auto city = df.features("city");
auto sales = df.features("sales");
auto case_axis = df.cases();

auto first_row = df.iloc(0);
auto first_value = df.at(0, "city");
```

Notes:

- `features("name")` returns a typed `Thread`
- `cases()` is the row axis, not just a display index
- `describe()` is the fastest way to check whether numeric casts and missing values look right

## Common Everyday Tasks

These are the patterns most people look for first when learning the day-to-day
table workflow.

### Select one column vs many columns

```cpp
auto one = df.features("sales");                // Thread
auto many = df.features({"city", "sales"});     // DataFrame
```

Rule of thumb:

- `features("name")` returns one `Thread`
- `features({"a", "b"})` returns a smaller `DataFrame`

### Rename one column vs many columns

```cpp
auto renamed_one = df.rename("sales", "revenue");

auto renamed_many = df.rename_features({
    {"sales", "revenue"},
    {"city", "location"},
});
```

Use `rename(...)` for one feature. Use `rename_features(...)` when you have a
mapping of old names to new names.

### Replace or create a column

```cpp
df.set("profit") = df.features("sales") - 60.0;

auto out = df.set_features(
    "profit",
    df.features("sales") - 60.0
);
```

Use `set(...) = ...` when you want assignment-style syntax. Use
`set_features(...)` when you prefer expression-style chaining.

### Clean a text column

```cpp
gradebook.set("OrgDefinedId") =
    gradebook.features("OrgDefinedId").as_note().strip().lstrip("#");
```

This is the direct table-cleaning pattern:

```python
df["OrgDefinedId"] = df["OrgDefinedId"].astype(str).str.strip().str.lstrip("#")
```

Use `.as_note()` when you want Note-specific text methods such as `strip`,
`lstrip`, and `rstrip`.

### Display Views

The table-style preview verbs return a `DataFrameView`, not a plain `DataFrame`.
That is what makes this pattern work:

```cpp
df.head();          // terminal preview
df.head().show();   // popup if GTK/WebKit exists, else terminal preview
df.view(10, 20).show();
df.describe().show();
```

If you keep the result, the stored type is still the preview wrapper:

```cpp
auto preview = df.head(5);
preview.show();
std::cout << preview << "\n";
```

If you explicitly want a materialized `DataFrame`, use `render=false`:

```cpp
pluss::table::DataFrame head_df = df.head(5, false);
```

For OS support, popup backends, `DisplayOptions`, and `title(...)` / `title_(...)`,
see `dataframe_display.md`.

## Cast And Derive Features

```cpp
auto work = csv_df
    .cast_timeline({"Business Case ID"})
    .astype({{"Lifecycle Cost", pluss::codem::ScalarType::Float64}})
    .assign({
        pluss::table::DataFrame::DeriveSpec{
            "Cost_plus_buffer",
            [](const pluss::table::DataFrame& f) {
              return pluss::table::Thread(f.features("Lifecycle Cost").as_series().add(500.0));
            },
        },
    });
```

## Filter, Query, And Sort

```cpp
auto positive = df.filter(df.features("sales").as_series().gt(95.0));

auto queried = df.query("sales > 95");

auto sorted = df.sort_values({"sales"}, false);

auto top3 = df.top("sales", 3);
```

Notes:

- use `filter(bool_mask)` when you already have a boolean `Series`
- use `query("...")` when the filter is easier to read as a table expression
- `top` and `bottom` are good for ranking tasks without writing extra sort/slice steps

Direct literal comparison is also supported for note-backed columns:

```cpp
auto laurels = df.filter(df.features("First Name") == "Laurel");
```

That matches every row whose `First Name` equals `"Laurel"`.

## Grouping, Pivoting, And Summaries

```cpp
auto grouped = df.group_by(
    {"city"},
    {{"avg_sales", "sales", pluss::table::DataFrame::GroupOp::Mean}},
    true,
    false);

auto pivoted = df.pivot_table(
    "city",
    "city",
    "sales",
    pluss::table::DataFrame::GroupOp::Mean,
    true,
    false);

auto stats = df.aggregate({
    {"sales_mean", "sales", pluss::table::DataFrame::SummaryOp::Mean},
    {"sales_max", "sales", pluss::table::DataFrame::SummaryOp::Max},
}, false);
```

## Missing Data And Rolling Windows

```cpp
auto cleaned = df.fillna(0.0, {"sales"});
auto dropped = df.dropna();
auto carried = df.ffill();

auto rolling = df.window({
    {"sales_roll3", "sales", pluss::table::ThreadWindowOp::RollingMean, 3, 1},
});
```

## Join And Combine

```cpp
auto left = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});

auto right = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
    {"manager", pluss::table::Thread(pluss::make_note({"Ama", "Kojo"}))},
});

auto joined = left.stitch_on({"city"}, right, {"city"}, "_r");
auto updated = left.update(right, true);
```

`stitch_on(...)` is the inner-match join helper. For general key-based merge/join
workflows, prefer `unify(...)`.

### Same-key merge

```cpp
auto df_ready = gradebook.unify(
    "OrgDefinedId",
    finals.features({"OrgDefinedId", "Final Points Grade"}),
    "",
    "left"
);
```

Use `right_on=""` when both frames use the same key name.

### Different-key merge

```cpp
auto df_ready = gradebook.unify(
    "OrgDefinedId",
    finals.features({"Username", "Final Points Grade"}),
    "Username",
    "left"
);
```

### Cardinality validation

```cpp
auto df_ready = gradebook.unify(
    "OrgDefinedId",
    finals.features({"Username", "Final Points Grade"}),
    "Username",
    "left",
    "_right",
    "many_to_one"
);
```

`validate` options:

- `"many_to_many"`: default; duplicates are allowed on both sides
- `"one_to_one"`: both sides must have unique keys
- `"one_to_many"`: left must be unique; right may repeat
- `"many_to_one"`: right must be unique; left may repeat

Important behavior:

- matching is by key value, not by row position
- `how="left"` preserves left-row order
- if a non-key right-hand feature name collides with a left-hand name,
  `right_suffix` is appended to the right-hand feature name

## Time-Aware Table Verbs

```cpp
auto schedule = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"value", pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}))},
});

auto hourly = schedule.resample("ts", {"value"}, "1H", "mean", 1, false);
auto floored = schedule.floor_time({"ts"}, "hour");
auto utc = schedule.to_utc({"ts"});
```

## SQL And IO

```cpp
auto result = df.sql().query("SELECT city, sales FROM self ORDER BY sales DESC");

result.save_csv("path/to/output.csv");
result.save_excel("path/to/output.xlsx");
result.save_json("path/to/output.json");
```
