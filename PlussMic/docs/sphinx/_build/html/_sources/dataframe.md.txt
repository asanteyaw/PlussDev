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
auto case_axis = df.caselbl();

auto first_row = df.case_study(pluss::table::pos(0));
auto first_value = df.case_study(
    pluss::table::pos(0),
    pluss::table::col("city"));
```

Notes:

- `features("name")` returns a typed `Thread`
- `caselbl()` is the row axis, not just a display index
- `case_study(...)` is the row/column selection surface
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

auto renamed_many = df.rename({
    {"sales", "revenue"},
    {"city", "location"},
});

auto renamed_all = df.rename({"id", "city", "revenue"});
```

Use `rename(...)` for one feature, a mapping of old names to new names, or a
full replacement of the feature axis.

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
    .cast({
        pluss::table::DataFrame::CastSpec{
            "Business Case ID", pluss::table::ThreadCastTarget::Timeline, {}},
    })
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

## Match Frames By Cases And Features

Frame-to-frame arithmetic and comparisons match both axes automatically. The
default is a stable union: left-side case and feature order is retained, then
right-only labels are appended. A value present on only one side is null in the
result.

```cpp
auto total = left.add(right);
```

Use `match(...)` when you need the matched operands themselves or want a
different rule:

```cpp
pluss::table::FrameMatchOptions rules;
rules.cases = pluss::table::FrameMatchRule::Common;
rules.features = pluss::table::FrameMatchRule::KeepLeft;

auto matched = left.match(right, rules);
auto total = matched.left.add(matched.right);
```

Available case and feature rules are `Unite`, `Common`, `KeepLeft`, and
`KeepRight`. Repeated cases pair by occurrence order; set
`rules.repeated_cases = pluss::table::RepeatedCaseRule::Reject` when repeated
case labels should fail instead.

## Extend And Unify Frames

Append cases vertically with feature-name matching:

```cpp
auto history = january.extend_cases(february);
```

The default feature rule is a stable union with nulls for one-sided features.
Pass `FrameMatchRule::Common` to retain shared features only, or set
`fresh_cases=true` for contiguous integer cases:

```cpp
auto compact = january.extend_cases(
    february,
    pluss::table::FrameMatchRule::Common,
    true);
```

Use `unify(...)` for key-based case combinations:

```cpp
auto matched = grades.unify("student_id", roster);                  // common
auto complete = grades.unify("student_id", roster, "", "unite");   // both sides
auto absent = grades.unify("student_id", roster, "", "left_only");
auto product = grades.unify("", roster, "", "cross");
```

The Pluss-native modes are `common`, `keep_left`, `keep_right`, `unite`,
`cross`, `left_only`, and `right_only`. Repeated keys expand to all matching
pairs. Use the `validate` argument when the relationship must be one-to-one,
one-to-many, or many-to-one.

## Grouped Reductions

One call can compute several named reductions:

```cpp
auto summary = sales.group_by(
    {"region"},
    {
        {"orders", "", pluss::table::DataFrame::GroupOp::Count},
        {"revenue", "amount", pluss::table::DataFrame::GroupOp::Sum},
        {"average", "amount", pluss::table::DataFrame::GroupOp::Mean},
        {"volatility", "amount", pluss::table::DataFrame::GroupOp::Std},
    },
    true,
    false);
```

Supported grouped operations are `Count`, `Nunique`, `Sum`, `Mean`, `Min`,
`Max`, `Std`, and `Var`. Numeric reductions use Series scatter operations and
stay on the payload platform. Group keys retain their original Thread type and
first-seen order.

## Wide-To-Long Reshape

`lengthen(...)` stacks homogeneous value features while repeating identifier
features:

```cpp
auto long_form = wide.lengthen(
    {"student_id"},
    {"midterm", "final"},
    "assessment",
    "score");
```

The result contains `student_id`, `assessment`, and `score`. Numeric value
features must share dtype and platform; other value features must share a
Thread kind. Missing masks are preserved. Leave the value-feature list empty
to stack every non-identifier feature.

## Low-Cardinality Encoding

Use `factor(...)` when repeated labels should have compact integer codes:

```cpp
auto city = df.factor("city");
auto codes = city.codes;
auto levels = city.levels;
auto restored = city.decode();
```

The encoding remains explicit so integer codes cannot be mistaken for measured
numeric data. Missing values use payload code `-1` and retain an invalid mask.
Levels preserve first-seen order unless `sort_levels=true`.

## Stream Large CSV Files By Cases

`CsvCaseReader` keeps the file open and parses only the next bounded case batch:

```cpp
pluss::loader::CsvCaseReader reader("path/to/large.csv", 100'000);
pluss::table::DataFrame batch;

while (reader.next(batch)) {
    process(batch);
}
```

The reader does not re-scan earlier records and does not materialize the whole
file. `ReadCsvOptions::ncases` acts as a global cap. Direct `load_csv(...)`
calls reject `chunksize` explicitly because a single `DataFrame` return value
cannot represent several streamed batches.

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
auto dropped = df.dropnull();
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

auto joined = left.unify({"city"}, right, {"city"}, "common", "_r");
auto updated = left.update(right, true);
```

`unify(...)` is the key-based merge/join helper. Use `update(...)` when the
right frame should patch matching cells/features.

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
