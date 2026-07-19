<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8705faaeef83 -->

# DataFrame::group_by

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrameView group_by(const LabelGroup& keys,
                        pluss::RangeView<GroupSpec> specs,
                        bool dropnull_keys = true,
                        bool render = false) const

DataFrameView group_by(pluss::RangeView<std::string_view> keys,
                        pluss::RangeView<GroupSpec> specs,
                        bool dropnull_keys = true,
                        bool render = false) const

DataFrameView group_by(std::initializer_list<std::string_view> keys,
                        pluss::RangeView<GroupSpec> specs,
                        bool dropnull_keys = true,
                        bool render = false) const
```

## What It Does

Groups cases by the distinct combinations of one or more key features, computes one or
more aggregations per group, and returns the result as a new, much smaller frame — one
row per distinct key combination, plus one column per requested aggregation, collapsed
into a single call instead of a separate group-then-aggregate pipeline.

Groups are formed in **stable first-seen order**: the first group in the result
corresponds to whichever key combination appears first in the source frame, not to a
sorted or hashed order. If you want the result sorted by key or by an aggregated value,
chain [`sort_values()`](sort_values.md) onto the result.

## Parameters

**keys** (`const LabelGroup&` / `pluss::RangeView<std::string_view>` /
`std::initializer_list<std::string_view>`)

Feature name(s) whose distinct value combinations define the output groups. A single key
groups by one feature's distinct values; multiple keys group by the distinct
combinations across all of them (a composite group key formed from every listed
feature).

**specs** (`pluss::RangeView<GroupSpec>`)

The aggregations to compute per group. Each `GroupSpec` names an output feature, the
source feature to aggregate, and an operation (`GroupOp`):

- `Count`: number of observed (non-null) values in the group.
- `Nunique`: number of distinct observed values in the group.
- `Sum`: numeric sum.
- `Mean`: arithmetic mean.
- `Min` / `Max`: extreme values.
- `Std` / `Var`: sample standard deviation / variance.

Every `GroupSpec` in `specs` is computed independently, so a single `group_by()` call can
mix operations across different source features — a sum of one column and a mean of
another in the same call.

**dropnull_keys** (`bool`, default `true`)

When `true`, cases with a null value in any key feature are excluded before grouping —
they contribute to no group at all. When `false`, a null key value is treated as its own
distinct group instead of being dropped.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, same convention as
[`head()`](head.md) / [`describe()`](describe.md): `false` gives you explicit control
over when the result is printed; `true` allows it to auto-render in a temporary-value
context.

## Returns

`DataFrameView`
    A new frame with one row per distinct key combination (in first-seen order) and one
    column per key feature followed by one column per `GroupSpec` output name. Aggregated
    numeric operations ignore null source values; count-style aggregations count only
    observed values.

## Examples

Example 1. Sum one feature and average another, grouped by a single key.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"city",       Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Kumasi", "Kumasi"}))},
    {"product",    Thread(pluss::make_note({"Laptop", "Headphones", "Laptop", "Monitor", "Headphones", "Monitor"}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
});

auto by_city = orders.group_by(
    {"city"},
    {
        GroupSpec{.output = "units_total", .feature = "units", .op = DataFrame::GroupOp::Sum},
        GroupSpec{.output = "avg_price",   .feature = "unit_price", .op = DataFrame::GroupOp::Mean},
    });
std::cout << by_city;
```

### Output

```text
=====================================
     | city    units_total  avg_price
-------------------------------------
0    | Accra   7            622.75   
1    | Tema    4            735      
2    | Kumasi  6            42       
-------------------------------------
[3 cs x 3 feat]
```

`Kumasi`'s `avg_price` is `42`, the mean of only its one *observed* price — case `106`'s
missing `unit_price` is skipped rather than treated as `0` or propagated as a null
result. `city` itself groups the six source rows into three groups, in the order each
city first appears in `orders` (`Accra`, then `Tema`, then `Kumasi`), not alphabetically.

Example 2. Group by more than one key at once.

```cpp
auto by_city_product = orders.group_by(
    {"city", "product"},
    {GroupSpec{.output = "units_total", .feature = "units", .op = DataFrame::GroupOp::Sum}});
```

`by_city_product` has one row per distinct `(city, product)` pair that actually occurs in
`orders` — combinations that never appear in the source frame (`"Accra", "Monitor"`, for
instance) simply have no row in the result, rather than a row with a zero or null total.
This is the key structural difference from [`pivot_table()`](pivot_table.md), which
reshapes one of the group keys into columns and therefore does materialize a cell —
typically null — for every combination, observed or not.

## Notes

- Count, sum, mean, min, max, and the sample variance/standard deviation aggregations
  all skip null source values automatically; they never need a separate `fillnull()`
  pass beforehand unless you specifically want nulls to participate as zeros.
- Because groups are in first-seen order rather than sorted order, always chain
  `sort_values()` explicitly when the output needs to be in a specific order (by key, or
  by one of the aggregated columns) — do not rely on incidental ordering.
- `dropnull_keys=false` is the switch to reach for when "unknown key" rows are
  meaningful and should be reported as their own group rather than silently excluded.

## See Also

- [`DataFrame::pivot_table`](pivot_table.md): the same aggregation idea, reshaped so one
  key becomes columns instead of staying a row-level group key.
- [`DataFrame::mean_by`](mean_by.md) / [`count_by`](count_by.md): single-aggregation
  shortcuts when you only need one statistic per group.
- [`DataFrame::sort_values`](sort_values.md): impose an explicit row order on the grouped
  result.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
