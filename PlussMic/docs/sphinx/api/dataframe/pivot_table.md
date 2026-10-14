<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62f6c6a1c7b7 -->

# DataFrame::pivot_table

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::pivot_table(std::string_view case_feature,
                       std::string_view feature_feature,
                       std::string_view value_feature,
                       GroupOp op = GroupOp::Mean,
                       bool dropnull_keys = true,
                       bool render = false) const -> DataFrameView

DataFrame::pivot_table(const PivotTableOptions& options) const -> DataFrameView

DataFrame::pivot_table(pluss::msharp::RangeView::Of<std::string_view> index_features,
                       pluss::msharp::RangeView::Of<std::string_view> column_features,
                       pluss::msharp::RangeView::Of<std::string_view> value_features,
                       GroupOp op = GroupOp::Mean,
                       bool dropna = true,
                       bool render = false,
                       bool sort = true) const -> DataFrameView

DataFrame::pivot_table(std::initializer_list<std::string_view> index_features,
                       std::initializer_list<std::string_view> column_features,
                       std::initializer_list<std::string_view> value_features,
                       GroupOp op = GroupOp::Mean,
                       bool dropna = true,
                       bool render = false,
                       bool sort = true) const -> DataFrameView
```

## What It Is

`pivot_table()` is the summarized wide reshape.

It starts from long data, groups by row-side keys and column-side keys, reduces one or
more value features inside each key combination, and then spreads the column-side key
combinations across wide output features.

If you know pandas, this is the Pluss analogue of `pandas.pivot_table(...)`. The main
idea is the same, but the rendering model is deliberately Pluss-native:

- row keys stay on the case axis
- multi-row keys become structured case labels
- multi-column key tuples are flattened into descriptive feature names
- the result is still an ordinary `DataFrame`, not a column `MultiIndex`

That means a pivot such as:

- row keys: `{"city", "quarter"}`
- column keys: `{"channel", "product"}`
- values: `{"units", "revenue"}`
- aggregations: `Sum`, `Mean`

still behaves like a full multi-key pivot, but the output features will read more like:

- `units_sum :: Card | Laptop`
- `units_mean :: Card | Laptop`
- `revenue_sum :: Cash | Phone`

instead of forcing a column hierarchy.

## Mental Model

Read `pivot_table()` as:

1. choose the case-side grouping keys with `index`
2. choose the feature-side grouping keys with `columns`
3. choose what to aggregate with `values` or `specs`
4. choose how to aggregate with `aggfuncs` or per-value `specs`
5. decide whether missing key combinations stay null or get filled
6. decide whether totals should be appended with `margins`

In other words, `pivot_table()` is for:

- duplicate `(row_key, column_key)` combinations
- long transactional or observational data
- spreadsheet-like summaries
- "show me the grouped values in matrix form"

It is not for:

- pure reshaping with guaranteed unique key pairs
- retained N-dimensional categorical objects
- arbitrary relational joins

Use nearby methods accordingly:

- use [`pivot()`](pivot.md) when each `(case_feature, feature_feature)` pair is already unique
- use [`group()`](group.md) when you want a grouped long result rather than a wide matrix
- use [`contingency()`](contingency.md) when the primary object is a retained contingency table, not an immediately rendered summary matrix

## How Pluss Renders The Result

This is one of the most important things to understand before you build larger pivots.

### Row keys

If `index = {"city"}`, you get one case per city.

If `index = {"city", "quarter"}`, Pluss keeps both keys on the case axis. When printed,
they appear as a combined case label using `index_separator`.

By default the separator is an internal control separator. For human-readable printed
output, it is often worth setting:

```cpp
options.index_separator = " / ";
```

### Column keys

If `columns = {"product"}`, each product becomes a feature or feature family.

If `columns = {"channel", "product"}`, each tuple such as `("Card", "Laptop")` becomes a
flattened label using `column_separator`, for example:

```text
Card | Laptop
```

### Value features and aggregation names

If there is more than one resolved value aggregation, Pluss prefixes the flattened column
label with the value-aggregation display name using `value_separator`, for example:

```text
revenue_sum :: Card | Laptop
```

If exactly one value aggregation is resolved, Pluss omits that prefix and uses only the
column label when that is unambiguous. This preserves the simple look of the old
single-value surface.

## Overload Guide

Use the overload that matches the job rather than forcing every call through the options
struct.

### Legacy three-key overload

Use this when you have:

- one row key
- one column key
- one value feature
- one aggregation

```cpp
df.pivot_table("city", "product", "units", DataFrame::GroupOp::Sum);
```

It is still fully supported. It is now just the simple front door into the generalized
pivot engine.

### Range or initializer-list overloads

Use these when you want:

- multiple row keys
- multiple column keys
- multiple value features
- one uniform aggregation applied to all selected values

```cpp
df.pivot_table({"city", "quarter"},
               {"product"},
               {"units", "revenue"},
               DataFrame::GroupOp::Sum);
```

This is the cleanest surface when every value feature should use the same reduction.

### `PivotTableOptions`

Use the options struct when you need any of the following:

- multiple aggregations
- per-value aggregation control
- `fill_value`
- `margins`
- custom `margins_name`
- custom separators
- explicit `sort`
- explicit `dropna`

This is the fully explicit surface and the one to reach for once your pivot becomes more
than a toy.

## The Full Options Surface

```cpp
struct PivotValueSpec {
  std::string output{};
  std::string feature{};
  GroupOp op{GroupOp::Mean};
};

struct PivotTableOptions {
  std::vector<std::string> index{};
  std::vector<std::string> columns{};
  std::vector<std::string> values{};
  std::vector<GroupOp> aggfuncs{GroupOp::Mean};
  std::vector<PivotValueSpec> specs{};
  std::optional<SlateValue::HostAny> fill_value{std::nullopt};
  bool margins{false};
  std::string margins_name{"All"};
  bool dropna{true};
  bool sort{true};
  bool render{false};
  std::string index_separator{"\x1F"};
  std::string column_separator{" | "};
  std::string value_separator{" :: "};
};
```

## Option Semantics

### `index`

The case-side grouping keys.

- one feature means one output row per distinct key
- multiple features mean one output row per distinct tuple
- when printed, multiple features are rendered using `index_separator`

### `columns`

The feature-side grouping keys.

- one feature means one output column family per distinct value
- multiple features mean one output column family per distinct tuple
- tuple labels are flattened using `column_separator`

### `values`

The value features to aggregate.

If omitted, `pivot_table()` tries to infer every non-key feature compatible with the
requested aggregation list. This is convenient for quick exploration, but it can widen
the result more than expected on large frames. For production pivots, explicit `values`
or `specs` are usually clearer.

### `aggfuncs`

Uniform aggregation functions applied to every selected value feature.

If `values = {"units", "revenue"}` and `aggfuncs = {Sum, Mean}`, Pluss resolves four
value aggregations:

- `units_sum`
- `units_mean`
- `revenue_sum`
- `revenue_mean`

### `specs`

Explicit per-output aggregation control.

Each `PivotValueSpec` says:

- which source feature to aggregate
- which aggregation to apply
- what output display name to use

If `specs` is non-empty, it takes precedence over `values` and `aggfuncs`.

Use it when you want "sum this, mean that, mode this note feature, count rows here"
instead of one aggregation rule for everything.

### `fill_value`

Applied after the pivot matrix is built.

Observed grouped cells keep their aggregated values. Only missing combinations are filled.
This matters because:

- null produced by a genuinely missing `(row_key, column_key)` combination can be filled
- null already present inside source values still follows the aggregation's own rules

### `margins`

Adds totals.

- if there is a column axis, extra total features are appended
- if there is a row axis, an extra bottom total row is appended
- if both axes exist, you get both row totals and column totals

### `margins_name`

Controls the label used for totals. Default is `"All"`.

If `margins=true`, `margins_name` cannot be empty.

### `dropna`

Controls null grouping keys.

- `true`: rows with null `index` or `columns` keys are excluded before grouping
- `false`: null keys become real groups and appear in the output

When rendered, null-like labels appear as `NULL`, consistent with the rest of the Pluss
table surface.

### `sort`

Controls grouped key ordering.

- `true`: distinct row-key tuples and column-key tuples are ordered lexicographically
- `false`: first-seen group order is preserved

This affects display order, not the aggregated values themselves.

### Separators

- `index_separator` controls how multi-row-key labels print
- `column_separator` controls how multi-column-key tuples flatten
- `value_separator` controls how value-aggregation labels join to flattened column labels

These are naming and rendering controls. They do not change grouping semantics.

## Aggregations And Type Rules

`pivot_table()` uses the same grouped reduction family as [`group().agg()`](group.md).

Supported grouped operations include:

- `Count`
- `Ndistinct`
- `Sum`
- `Mean`
- `Median`
- `Min`
- `Max`
- `Std`
- `Var`
- `Mode`

Type rules matter:

- `Sum`, `Mean`, `Median`, `Min`, `Max`, `Std`, and `Var` require numeric value features
- `Mode` works on note features and numeric series
- `Count` can operate without a concrete value feature

That last point is important. The generalized pivot engine supports row counting even when
you do not name a value feature explicitly, but only for `Count`.

## Failure Modes And Guardrails

`pivot_table()` is permissive where it is safe and strict where ambiguity would create bad
results.

It rejects these situations:

- no `index` and no `columns`
- the same feature appearing on both axes
- `margins=true` with an empty `margins_name`
- non-numeric features passed to numeric aggregations
- non-`Count` specs with an empty source feature
- a resolved pivot with no compatible value aggregations

A few behavior details are worth knowing:

- if `values` is omitted, inference only keeps features compatible with every requested aggregation
- if inference finds nothing and the only requested aggregation is `Count`, Pluss falls back to row counting
- if output feature names would collide, Pluss uniquifies them rather than silently overwriting

## Examples

The examples below build from simple to advanced. Read them in order if you are learning
the surface for the first time.

Example 1. Classic one-row-key, one-column-key pivot using the legacy overload.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city",    Slate(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"product", Slate(pluss::make_note({"Laptop", "Laptop", "Phone", "Laptop", "Phone"}))},
    {"units",   Slate(pluss::make_series({2.0, 4.0, 1.0, 3.0, 5.0}))},
});

auto out = sales.pivot_table("city", "product", "units", DataFrame::GroupOp::Sum);
std::cout << out;
```

### Output

```text
=============================
      | Laptop  Phone
---------------------
Accra | 6       1
Tema  | 3       5
---------------------
[2 cs x 2 feat]
```

This is the original `pivot_table()` use case. Duplicate `(city, product)` pairs exist,
so a plain [`pivot()`](pivot.md) would be wrong. The aggregation makes the collapse
explicit.

Example 2. Default aggregation is `Mean`, not `Sum`.

```cpp
auto avg = sales.pivot_table("city", "product", "units");
std::cout << avg;
```

### Output

```text
=============================
      | Laptop  Phone
---------------------
Accra | 3       1
Tema  | 3       5
---------------------
[2 cs x 2 feat]
```

The repeated `Accra`/`Laptop` values `2` and `4` are averaged to `3`. This is one of the
most important behavioral differences between `pivot_table()` and `pivot()`.

Example 3. Multiple row keys and multiple value features with one uniform aggregation.

```cpp
auto detailed = DataFrame({
    {"city",    Slate(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"quarter", Slate(pluss::make_note({"Q1", "Q1", "Q2", "Q1", "Q2"}))},
    {"product", Slate(pluss::make_note({"Laptop", "Phone", "Laptop", "Laptop", "Phone"}))},
    {"units",   Slate(pluss::make_series({2.0, 3.0, 4.0, 1.0, 5.0}))},
    {"revenue", Slate(pluss::make_series({20.0, 30.0, 40.0, 10.0, 50.0}))},
});

DataFrame::PivotTableOptions options;
options.index = {"city", "quarter"};
options.columns = {"product"};
options.values = {"units", "revenue"};
options.aggfuncs = {DataFrame::GroupOp::Sum};
options.index_separator = " / ";

auto multi = detailed.pivot_table(options);
std::cout << multi;
```

### Output

```text
=================================================================================================
           | units_sum :: Laptop  units_sum :: Phone  revenue_sum :: Laptop  revenue_sum :: Phone
-------------------------------------------------------------------------------------------------
Accra / Q1 | 2                    3                   20                     30
Accra / Q2 | 4                    NULL                40                     NULL
Tema / Q1  | 1                    NULL                10                     NULL
Tema / Q2  | NULL                 5                   NULL                   50
-------------------------------------------------------------------------------------------------
[4 cs x 4 feat]
```

This example shows three things at once:

- multiple `index` features create a tuple-grain row axis
- multiple `values` create multiple output feature families
- missing `(row_key, column_key)` combinations stay null unless you fill them

Example 4. Multiple aggregation functions applied uniformly to every selected value.

```cpp
DataFrame::PivotTableOptions multiagg;
multiagg.index = {"city"};
multiagg.columns = {"product"};
multiagg.values = {"units", "revenue"};
multiagg.aggfuncs = {
    DataFrame::GroupOp::Sum,
    DataFrame::GroupOp::Mean,
};

auto rich = detailed.pivot_table(multiagg);
std::cout << rich;
```

### Output

```text
=====================================================================================================================================================
      | units_sum :: Laptop  units_mean :: Laptop  units_sum :: Phone  units_mean :: Phone  revenue_sum :: Laptop  revenue_mean :: Laptop  revenue_sum :: Phone  revenue_mean :: Phone
-----------------------------------------------------------------------------------------------------------------------------------------------------
Accra | 6                    3                     3                   3                    60                     30                       30                    30
Tema  | 1                    1                     5                   5                    10                     10                       50                    50
-----------------------------------------------------------------------------------------------------------------------------------------------------
[2 cs x 8 feat]
```

This is the closest analogue to pandas `aggfunc=["sum", "mean"]`. Every selected value
feature is expanded against every requested aggregation.

Example 5. Use explicit `specs` when different value features need different reductions.

```cpp
auto events = DataFrame({
    {"city",    Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Tema"}))},
    {"channel", Slate(pluss::make_note({"Card", "Cash", "Card", "Card", "Cash"}))},
    {"amount",  Slate(pluss::make_series({20.0, 35.0, 10.0, 50.0, 40.0}))},
    {"status",  Slate(pluss::make_note({"ok", "ok", "retry", "ok", "retry"}))},
});

DataFrame::PivotTableOptions mixed;
mixed.index = {"city"};
mixed.columns = {"channel"};
mixed.specs = {
    {"amount_sum", "amount", DataFrame::GroupOp::Sum},
    {"status_mode", "status", DataFrame::GroupOp::Mode},
};

auto mixed_out = events.pivot_table(mixed);
std::cout << mixed_out;
```

### Output

```text
========================================================================================
      | amount_sum :: Card  amount_sum :: Cash  status_mode :: Card  status_mode :: Cash
----------------------------------------------------------------------------------------
Accra | 20                  35                  ok                   ok
Tema  | 60                  40                  retry                retry
----------------------------------------------------------------------------------------
[2 cs x 4 feat]
```

This is the right surface when you need different reductions for different features. It
is also how you pivot note data safely through `Mode` without forcing everything through a
numeric aggregation.

Example 6. `fill_value` and `margins` make the result closer to a spreadsheet pivot.

```cpp
options.fill_value = SlateValue::HostAny::floating(0.0);
options.margins = true;

auto totaled = detailed.pivot_table(options);
std::cout << totaled;
```

### Output

```text
=======================================================================================================================================
           | units_sum :: Laptop  units_sum :: Phone  units_sum :: All  revenue_sum :: Laptop  revenue_sum :: Phone  revenue_sum :: All
---------------------------------------------------------------------------------------------------------------------------------------
Accra / Q1 | 2                    3                   5                 20                     30                    50
Accra / Q2 | 4                    0                   4                 40                     0                     40
Tema / Q1  | 1                    0                   1                 10                     0                     10
Tema / Q2  | 0                    5                   5                 0                      50                    50
All /      | 7                    8                   15                70                     80                    150
---------------------------------------------------------------------------------------------------------------------------------------
[5 cs x 6 feat]
```

Two behaviors matter here:

- missing combinations are filled after pivoting
- totals are appended both as extra wide features and as an extra bottom row

Example 7. `dropna=false` keeps null key groups instead of discarding them.

```cpp
auto null_keys = DataFrame({
    {"city",    Slate(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}), {true, false, true, true})},
    {"product", Slate(pluss::make_note({"Laptop", "Laptop", "Phone", "Phone"}))},
    {"units",   Slate(pluss::make_series({2.0, 4.0, 5.0, 1.0}))},
});

auto keep_null = null_keys.pivot_table("city",
                                       "product",
                                       "units",
                                       DataFrame::GroupOp::Sum,
                                       false);
std::cout << keep_null;
```

### Output

```text
=============================
      | Laptop  Phone
---------------------
Accra | 2       NULL
Tema  | NULL    6
NULL  | 4       NULL
---------------------
[3 cs x 2 feat]
```

This is useful when null itself is analytically meaningful and should remain visible as a
group rather than being filtered out.

Example 8. Multi-column tuples flatten with `column_separator`, and custom names make
wide outputs easier to read.

```cpp
DataFrame::PivotTableOptions flat;
flat.index = {"city"};
flat.columns = {"quarter", "product"};
flat.specs = {
    {"units_total", "units", DataFrame::GroupOp::Sum},
};
flat.column_separator = " -> ";

auto flattened = detailed.pivot_table(flat);
std::cout << flattened;
```

### Output

```text
===================================================================================================
      | Q1 -> Laptop  Q1 -> Phone  Q2 -> Laptop  Q2 -> Phone
---------------------------------------------------------------------------------------------------
Accra | 2            3            4             NULL
Tema  | 1            NULL         NULL          5
---------------------------------------------------------------------------------------------------
[2 cs x 4 feat]
```

Because only one value aggregation is resolved, Pluss omits the `units_total` prefix and
uses only the flattened column tuple labels. That keeps the matrix compact while still
respecting the full pivot semantics.

Example 9. Counting rows without naming a value feature explicitly.

```cpp
DataFrame::PivotTableOptions counts;
counts.index = {"city"};
counts.columns = {"product"};
counts.aggfuncs = {DataFrame::GroupOp::Count};

auto counted = sales.pivot_table(counts);
std::cout << counted;
```

### Output

```text
=============================
      | Laptop  Phone
---------------------
Accra | 2       1
Tema  | 1       1
---------------------
[2 cs x 2 feat]
```

This is the special inferred-count case. Because no explicit values were supplied and the
only aggregation is `Count`, the pivot resolves to row counts for each key combination.

## Practical Guidance

For real work, these habits usually produce the clearest pivots:

- start with explicit `index`, `columns`, and `values`
- use one uniform aggregation first so you can inspect shape quickly
- switch to `specs` once different features need different reductions
- set `index_separator` to a visible token when printing multi-row pivots
- use `fill_value` only when missing combinations should be treated as actual zeros or placeholders
- use `margins` deliberately, because totals widen the result and can obscure the core matrix if overused

## Notes

- `pivot_table()` returns a `DataFrameView`, so it integrates with normal rendering and `.show()` behavior
- multi-row pivots use the case axis natively; use `label().reset()` only when you explicitly want the row hierarchy back as ordinary features
- null cells render as `NULL`, consistent with the rest of the dataframe surface
- output feature names are flattened intentionally so the result stays a normal `DataFrame`

## See Also

- [`DataFrame::pivot`](pivot.md)
- [`DataFrame::group`](group.md)
- [`DataFrame::contingency`](contingency.md)
- [`DataFrame::stack`](stack.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pivot.html">
    <span class="label">Previous</span>
    <span class="title">pivot</span>
  </a>
  <a class="next-link" href="place.html">
    <span class="label">Next</span>
    <span class="title">place</span>
  </a>
</div>
