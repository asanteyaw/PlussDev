<!-- pluss-hand-authored -->

# DataFrame::pivot

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::pivot(std::string_view case_feature,
                 std::string_view feature_feature,
                 std::string_view value_feature,
                 bool dropnull_keys = true,
                 bool render = false) const -> DataFrameView
```

## What It Does

Reshapes long-format data into a wide table — one row per distinct `case_feature`
value, one column per distinct `feature_feature` value, each cell holding the single
`value_feature` observation for that `(case_feature, feature_feature)` pair. This is
the "no aggregation needed" reshape: every `(case_feature, feature_feature)` pair in
the source data must occur **at most once**, or `pivot()` throws — "use pivot_table
with an aggregation op." [`pivot_table()`](pivot_table.md) is the sibling that
accepts (and requires you to name) an aggregation for when pairs repeat.

Internally, `pivot()` first checks for duplicate pairs via a `Count` group, then
delegates to `pivot_table(..., GroupOp::Max, ...)` — `Max` is a safe no-op aggregation
when every group has exactly one member.

## Parameters

**case_feature** (`std::string_view`)

Feature whose distinct values become the output rows.

**feature_feature** (`std::string_view`)

Feature whose distinct values become the new output columns.

**value_feature** (`std::string_view`)

Feature whose single value per `(case_feature, feature_feature)` pair populates each
cell.

**dropnull_keys** (`bool`, default `true`)

When `true`, rows with a null `case_feature` or `feature_feature` value are excluded
before pivoting. When `false`, a null key value forms its own row or column instead.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`; `false` for explicit printing, `true`
to allow auto-render in a temporary-value context.

## Returns

`DataFrameView`
    A new frame with one row per distinct `case_feature` value, `case_feature` itself
    as the first column, and one additional column per distinct `feature_feature`
    value. A cell for a `(case_feature, feature_feature)` combination that never
    occurs in the source data is `NULL`.

## Examples

Example 1. Pivot per-order sales into a city-by-product grid, where every
`(city, product)` pair happens to be unique.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",     Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",  Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"sales",    Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0, 80.0, 60.0}))},
});

auto by_city_product = orders.pivot("city", "product", "sales");
std::cout << by_city_product;
```

### Output

```text
==========================================
     | city    Laptop  Headphones  Monitor
------------------------------------------
0    | Accra   100     120         NULL   
1    | Tema    90      NULL        150    
2    | Kumasi  NULL    80          60     
------------------------------------------
[3 cs x 4 feat]
```

Every `(city, product)` pair in `orders` occurs exactly once, so `pivot()` can spread
`sales` out directly with no aggregation choice to make. Pairs that never occur —
Accra never sold a `Monitor`, Tema never sold `Headphones`, Kumasi never sold a
`Laptop` — show up as `NULL`, not `0`.

Example 2. A repeated pair throws, pointing you at `pivot_table()` instead.

```cpp
auto orders_with_repeat = DataFrame({
    {"order_id", Thread(pluss::make_series({101, 102, 103}))},
    {"city",     Thread(pluss::make_note({"Accra", "Accra", "Tema"}))},
    {"product",  Thread(pluss::make_note({"Laptop", "Laptop", "Laptop"}))},
    {"sales",    Thread(pluss::make_series({100.0, 200.0, 90.0}))},
});

try {
  auto out = orders_with_repeat.pivot("city", "product", "sales");
} catch (const pluss::Error& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
DataFrame::pivot: duplicate case/feature pairs found; use pivot_table with an aggregation op
```

`(Accra, Laptop)` occurs twice (orders `101` and `102`) — `pivot()` has no rule for
combining two `sales` values into one cell, so it throws rather than silently picking
one. [`pivot_table()`](pivot_table.md) with an explicit `GroupOp` (`Sum`, `Mean`, and
so on) is the reshape to reach for once pairs can repeat.

## Notes

- `pivot()` throws if any `(case_feature, feature_feature)` pair occurs more than
  once in the source data — it never silently picks one observation or averages
  them. Use [`pivot_table()`](pivot_table.md) with an explicit aggregation for that.
- A pair that never occurs in the source data becomes `NULL` in the result, never `0`.
- `dropnull_keys = false` keeps a null `case_feature` or `feature_feature` value as its
  own row or column rather than silently dropping those cases.

## See Also

- [`DataFrame::pivot_table`](pivot_table.md): the aggregating sibling, required once
  `(case_feature, feature_feature)` pairs can repeat.
- [`DataFrame::group_by`](group_by.md): the long-format equivalent, keeping every key
  as its own row feature instead of spreading one key into columns.
- [`DataFrame::stack`](stack.md): fold pivoted columns back into a long format.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="percentiles.html">
    <span class="label">Previous</span>
    <span class="title">percentiles</span>
  </a>
  <a class="next-link" href="pivot_table.html">
    <span class="label">Next</span>
    <span class="title">pivot_table</span>
  </a>
</div>
