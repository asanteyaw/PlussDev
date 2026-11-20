<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a175154e0210 -->

# DataFrame::min

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
Shelf DataFrame::min(Axis axis = Axis::Case, bool skipnull = true) const
```

## What It Does

Reduces every numeric feature to its smallest observed value, returning one `Shelf`
whose entries line up with the frame's numeric feature names — the same shape
[`max()`](max.md), [`sum()`](sum.md), and [`mean()`](mean.md) return. Non-numeric
features are skipped entirely rather than compared as an error.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one minimum per feature. `Axis::Feature` reduces across each row's features
instead, producing one value per case.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that feature's result:

- `true` (the default): missing values are excluded from the comparison entirely, so the
  minimum is whatever was actually observed.
- `false`: a single missing value in a feature poisons that feature's result, which comes
  back `NULL`.

## Returns

`Shelf`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's smallest observed value.

## Examples

Example 1. Find the minimum of every numeric feature, skipping missing values (the
default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Shelf(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Shelf(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Shelf(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, 0.0}), pluss::make_series(std::vector<bool>{true, true, true, true, true, false}))},
    {"discount",   Shelf(pluss::make_series({0.1, 0.05, 0.0, 0.08, 0.05, 0.12}), pluss::make_series(std::vector<bool>{true, true, false, true, true, true}))},
});

auto smallest = orders.min();
std::cout << smallest;
```

### Output

```text
  order_id  101
     units  1
unit_price  42
  discount  0.05
ptype: Series
```

`unit_price`'s missing observation (case `5`) is simply excluded from the comparison —
its minimum of `42` is the smallest of the five values that were actually observed, not a
`NULL` result.

Example 2. Disable null-skipping to see a missing value propagate.

```cpp
auto smallest_strict = orders.min(Axis::Case, false);
std::cout << smallest_strict;
```

### Output

```text
  order_id  101
     units  1
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, `unit_price` and `discount` come back `NULL` entirely rather
than the minimum of their remaining observed values. `order_id` and `units`, with no
missing values, are unaffected either way.

## Notes

- `min()`'s result is labeled by feature name and prints like a small labeled `Series`,
  not like a row sliced from the source frame.
- Non-numeric features never appear in the result; text and time features are not
  compared as an error, they are simply absent from the returned `Shelf`.
- `min()` reports the smallest *value*, not the case it came from — pair it with
  [`arrange()`](arrange.md) or a boolean mask built from
  [`filter()`](filter.md) if you also need the row that produced it.

## See Also

- [`DataFrame::max`](max.md): the corresponding largest observed value, sharing the same
  `axis` / `skipnull` shape.
- [`DataFrame::describe`](describe.md): reports both `min` and `max` alongside several
  other statistics in one call.
- [`DataFrame::clip`](clip.md): bound every numeric feature's values into a range, rather
  than reporting the range's current extremes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="median.html">
    <span class="label">Previous</span>
    <span class="title">median</span>
  </a>
  <a class="next-link" href="mod.html">
    <span class="label">Next</span>
    <span class="title">mod</span>
  </a>
</div>
