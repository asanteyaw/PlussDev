<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 92eb84b065e5 -->

# DataFrame::count

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::count(Axis axis = Axis::Case) const -> Slate
```

## What It Does

Counts non-missing entries along the requested axis and returns one `Slate` of
`Int64` counts. Unlike [`sum()`](sum.md), [`mean()`](mean.md), and the other numeric
reducers, `count()` does not skip non-numeric features — every feature in the frame
participates, because counting present-vs-missing cells doesn't require numeric
semantics the way averaging or summing does. There is also no `skipnull` parameter:
counting *is* the null-aware operation, so there is nothing to toggle.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) counts down each feature's cases,
producing one count per feature — how many of that column's cells are non-null.
`Axis::Feature` counts across each row's features instead, producing one count per
case — how many of that row's cells (across every feature) are non-null.

## Returns

`Slate`
    One `Int64` entry per feature (`Axis::Case`) or per case (`Axis::Feature`),
    holding the number of non-missing cells counted.

## Examples

Example 1. Count non-missing cells per feature, the default axis.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto counts = orders.count();
std::cout << counts;
```

### Output

```text
  order_id  6
     units  6
unit_price  5
  discount  5
ptype: Series
```

`order_id` and `units` have no missing cells, so both count all six cases. `unit_price`
and `discount` each have exactly one missing observation — case `5` and case `2`
respectively, the same two cells [`mean()`](mean.md) and [`std()`](std.md) exclude from
their statistics — so each counts only five.

`Axis::Case` is the default, so passing it explicitly changes nothing:

```cpp
auto counts_explicit = orders.count(Axis::Case);
std::cout << counts_explicit;
```

### Output

```text
  order_id  6
     units  6
unit_price  5
  discount  5
ptype: Series
```

Byte-for-byte the same result as the bare `orders.count()` call above.

Example 2. Count non-missing features per case with `Axis::Feature`.

```cpp
auto counts_by_case = orders.count(Axis::Feature);
std::cout << counts_by_case;
```

### Output

```text
0  4
1  4
2  3
3  4
4  4
5  3
ptype: Series
```

Every case has four features. Case `2` counts only `3` because its `discount` cell is
missing; case `5` counts only `3` because its `unit_price` cell is missing. Every other
case has all four cells populated and counts `4`.

Example 3. `count()` includes non-numeric features — unlike `sum()` or `mean()`, a
`Note` column is counted, not skipped.

```cpp
auto status_valid = pluss::make_series({true, true, true, true, false, true}, pluss::ScalarType::Bool);
auto orders_status = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
    {"status",     Slate(pluss::make_note({"shipped", "shipped", "shipped", "shipped", "", "shipped"}), status_valid)},
});

auto counts_all = orders_status.count();
std::cout << counts_all;
```

### Output

```text
  order_id  6
     units  6
unit_price  5
  discount  5
    status  5
ptype: Series
```

`status` (a `Note` column) appears in the result and reports `5`, correctly excluding
its one missing entry at case `4`. Compare this to [`sum()`](sum.md) or
[`mean()`](mean.md) on the same frame: those methods would silently drop `status`
from the result entirely rather than count it.

## Notes

- `count()`'s result is labeled by feature name (`Axis::Case`) or by case index
  (`Axis::Feature`), and prints like a small labeled `Series`, not like a row sliced
  from the source frame.
- Every feature participates regardless of type — `Series`, `Note`, `Timeline`, and
  `TimeIndex` columns are all counted. This is the one numeric-style reducer that does
  not restrict itself to numeric features.
- There is no `skipnull` parameter because there is nothing to skip: `count()` always
  reports the non-null count, never a poisoned `NULL` result.
- For counting distinct values rather than non-null ones, see
  [`ndistinct()`](ndistinct.md). For the most frequent values, see [`freq()`](freq.md).

## See Also

- [`DataFrame::sum`](sum.md), [`DataFrame::mean`](mean.md): numeric-only reducers that
  skip non-numeric features rather than counting them.
- [`DataFrame::ndistinct`](ndistinct.md): counts unique values instead of non-null ones.
- [`DataFrame::isnull`](isnull.md) / [`DataFrame::notnull`](notnull.md): the
  cell-by-cell mask this method effectively summarizes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr_with.html">
    <span class="label">Previous</span>
    <span class="title">corr_with</span>
  </a>
  <a class="next-link" href="covar.html">
    <span class="label">Next</span>
    <span class="title">covar</span>
  </a>
</div>
