<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6683526b0e89 -->

# DataFrame::mean

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::mean(Axis axis = Axis::Case, bool skipnull = true) const -> Thread
```

## What It Does

Reduces every numeric feature to its arithmetic mean, returning one `Thread` whose
entries line up with the frame's numeric feature names — the column-wise mean, one
number per feature, the same shape `DataFrame::sum()` returns. Non-numeric features are
skipped entirely rather than averaged into a meaningless result.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) averages down each feature's cases,
producing one mean per feature. `Axis::Feature` averages across each row's features
instead, producing one mean per case.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that feature's mean:

- `true` (the default): missing values are excluded from both the sum and the count, so
  the mean reflects only the observed values.
- `false`: a single missing value in a feature poisons that feature's mean entirely,
  which comes back `NULL`.

## Returns

`Thread`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's mean.

## Examples

Example 1. Average every numeric feature, skipping missing values (the default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Thread(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Thread(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Thread(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Thread(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto averages = orders.mean();
std::cout << averages;
```

### Output

```text
  order_id  103.5
     units  2.833333333
unit_price  551.5
  discount  0.08
ptype: Series
```

`unit_price`'s mean (`551.5`) divides by `5`, not `6` — its one missing observation
(case `5`) is excluded from both the running sum and the count used to divide it, the
same null-skipping rule [`describe()`](describe.md) applies to its own `mean` row. These
are in fact the exact same numbers `describe()` reports for this frame; `mean()` gives
you just that one statistic without computing the other seven.

Example 2. See a missing value poison the mean when null-skipping is disabled.

```cpp
auto averages_strict = orders.mean(pluss::table::Axis::Case, false);
std::cout << averages_strict;
```

### Output

```text
  order_id  103.5
     units  2.833333333
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, any feature that has even one missing value averages to `NULL`
rather than silently averaging over a smaller sample — useful when a missing value
should be a hard stop rather than something quietly absorbed into "however many values
happened to be observed."

## Notes

- `mean()`'s result is labeled by feature name, and prints like a small labeled `Series`
  (an index column plus a `ptype:` footer), not like a row sliced from the source frame.
- Non-numeric features never appear in the result; they are neither averaged as an error
  nor silently coerced.
- If you need several statistics at once (mean, spread, quartiles), reach for
  [`describe()`](describe.md) instead of calling `mean()`, `std()`, and
  `percentiles()` separately.

## See Also

- [`DataFrame::sum`](sum.md), [`DataFrame::std`](std.md), [`DataFrame::median`](median.md):
  the other single-statistic feature reductions, sharing the same `axis` / `skipnull`
  shape.
- [`DataFrame::describe`](describe.md): several statistics per feature in one call.
- [`DataFrame::group`](group.md): compute a grouped mean per group instead of one mean
  for the whole frame — `.group().by({...}).mean()` or an explicit `.agg({...})` spec.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="median.html">
    <span class="label">Next</span>
    <span class="title">median</span>
  </a>
</div>
