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

`pivot_table()` reshapes long data into a summarized matrix.

Conceptually it does three things:

1. groups rows by one or more row keys (`index`) and one or more column keys (`columns`)
2. aggregates one or more value features inside each key combination
3. spreads the column-side key combinations across wide output features

If you know pandas, this is the Pluss analogue of `pandas.pivot_table(...)`. The
generalized Pluss surface now covers the same core use cases:

- multiple row keys
- multiple column keys
- multiple value features
- multiple aggregation functions
- per-value aggregation specs
- `fill_value`
- `margins`
- `margins_name`
- `dropna`
- `sort`

The main representation difference is output shape:

- pandas uses row/column `MultiIndex`
- Pluss keeps multi-row keys as a structured case axis
- Pluss flattens multi-column combinations into descriptive feature names

That is why a multi-key pivot in Pluss still behaves like an N-key pivot, but prints as a
regular `DataFrame` with names such as `units_sum :: Laptop` or
`revenue_mean :: West | Retail`.

## When To Use It

Use `pivot_table()` when your source data is long and duplicate `(row_key, column_key)`
pairs need aggregation.

Prefer nearby methods in these cases:

- use [`pivot()`](pivot.md) when each `(case_feature, feature_feature)` pair is already
  unique and no aggregation should happen
- use [`group()`](group.md) when you want a long grouped result rather than a wide matrix
- use [`contingency()`](contingency.md) when the core object is a retained categorical
  joint distribution rather than an immediately rendered wide table

## Surface Summary

The legacy overload is still there:

```cpp
df.pivot_table("city", "product", "units", DataFrame::GroupOp::Sum);
```

It is now just the simple entry point into the generalized engine.

For the full surface, use `PivotTableOptions`:

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
  std::optional<ThreadValue::HostAny> fill_value{std::nullopt};
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

## How The Options Behave

**index**

The row-side grouping keys. With one feature, the output has one row per distinct key
value. With multiple features, the output keeps them as a structured case axis.

**columns**

The column-side grouping keys. With one feature, each distinct value becomes a set of
output columns. With multiple features, each distinct tuple is flattened into one display
label using `column_separator`.

**values**

The value features to aggregate. If omitted, `pivot_table()` infers every non-key feature
compatible with the requested aggregation(s).

**aggfuncs**

Uniform aggregation(s) applied to every feature in `values`. This is the closest analogue
to pandas' `aggfunc=["sum", "mean"]`.

**specs**

Explicit per-output aggregation specs. This is the targeted surface for "apply different
reductions to different features" rather than using one `aggfunc` across everything.

If `specs` is non-empty, it takes precedence over `values` and `aggfuncs`.

**fill_value**

Applied after the pivot is built. Unobserved cells stay null unless you supply a fill.

**margins / margins_name**

Adds totals. When a column axis exists, totals become extra wide features such as
`units_sum :: All`. When a row axis exists, an extra bottom row is appended.

**dropna**

If `true`, rows with null row/column keys are excluded before grouping. If `false`, null
keys become real groups and render as `NULL`/`<NULL>` labels where appropriate.

**sort**

If `true`, the grouped keys are ordered lexicographically before the pivot is spread wide.
If `false`, first-seen group order is preserved.

**index_separator / column_separator / value_separator**

These are formatting controls for multi-key rendering:

- `index_separator` is the join token used when structured row-key labels are rendered
- `column_separator` joins multiple column-side key levels in the flattened output label
- `value_separator` joins the value-aggregation label to the flattened column label

## Aggregations

`pivot_table()` accepts the same grouped reductions as [`group().agg()`](group.md):

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

The same type rules apply here too:

- `Sum`/`Mean`/`Median`/`Min`/`Max`/`Std`/`Var` require numeric value features
- `Mode` works on note features and on numeric series
- `Count` can count rows without needing a numeric value magnitude

## Examples

Example 1. Classic one-row-key, one-column-key pivot using the legacy overload.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city",    Thread(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"product", Thread(pluss::make_note({"Laptop", "Laptop", "Phone", "Laptop", "Phone"}))},
    {"units",   Thread(pluss::make_series({2.0, 4.0, 1.0, 3.0, 5.0}))},
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

This is the original `pivot_table()` use case: duplicate `(city, product)` pairs exist,
so a plain [`pivot()`](pivot.md) would be wrong, but `Sum` makes the intended collapse
explicit.

Example 2. Default aggregation is `Mean`, so repeated pairs are averaged rather than just
spread wide.

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

The two `Accra`/`Laptop` rows (`2` and `4`) are averaged to `3`. This is the key
behavioral difference between `pivot_table()` and a non-aggregating reshape.

Example 3. Multiple row keys, one column key, and multiple value features.

```cpp
DataFrame::PivotTableOptions options;
options.index = {"city", "quarter"};
options.columns = {"product"};
options.values = {"units", "revenue"};
options.aggfuncs = {DataFrame::GroupOp::Sum};
options.index_separator = " / ";

auto detailed = DataFrame({
    {"city",    Thread(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
    {"quarter", Thread(pluss::make_note({"Q1", "Q1", "Q2", "Q1", "Q2"}))},
    {"product", Thread(pluss::make_note({"Laptop", "Phone", "Laptop", "Laptop", "Phone"}))},
    {"units",   Thread(pluss::make_series({2.0, 3.0, 4.0, 1.0, 5.0}))},
    {"revenue", Thread(pluss::make_series({20.0, 30.0, 40.0, 10.0, 50.0}))},
});

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

The returned object keeps `{"city", "quarter"}` as a structured case axis. The printed row
labels are joined with `index_separator`, which is why this example sets it to `" / "`
instead of leaving the default internal separator.

Example 4. `fill_value` and `margins` make the result closer to a spreadsheet-style pivot.

```cpp
options.fill_value = ThreadValue::HostAny::floating(0.0);
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

This demonstrates two separate behaviors:

- missing combinations are filled after pivoting, so absent `Phone` cells become `0`
- totals are appended both across columns (`:: All`) and across rows (the bottom `All`
  case)

Example 5. Use explicit `specs` when different value features need different reductions.

```cpp
auto events = DataFrame({
    {"city",    Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Tema"}))},
    {"channel", Thread(pluss::make_note({"Card", "Cash", "Card", "Card", "Cash"}))},
    {"amount",  Thread(pluss::make_series({20.0, 35.0, 10.0, 50.0, 40.0}))},
    {"status",  Thread(pluss::make_note({"ok", "ok", "retry", "ok", "retry"}))},
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

This is the `pivot_table()` equivalent of pandas' "dict aggfunc" style. The two outputs
are not forced through the same reduction:

- `amount` is summed
- `status` is reduced by mode

## Notes

- If exactly one value spec is resolved, Pluss omits the value prefix and uses just the
  flattened column labels when that is unambiguous, preserving the old simple look.
- If `values` is omitted, compatible non-key features are inferred automatically.
- Multi-column pivots stay ordinary `DataFrame`s. Pluss does not manufacture a column
  `MultiIndex`; it flattens column tuples into stable feature names instead.
- Multi-row pivots use the case axis natively. Use `label().reset()` only when you want
  the row hierarchy materialized as ordinary features for printing or downstream joins.
- Missing cells are true nulls unless you explicitly provide `fill_value`.

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
