<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1c8606f8d750 -->

# DataFrame::median

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
Slate DataFrame::median(Axis axis = Axis::Case, bool skipnull = true) const
```

## What It Does

Reduces every numeric feature to its median, returning one `Slate` whose entries line
up with the frame's numeric feature names — the same shape [`mean()`](mean.md) and
[`sum()`](sum.md) return. Non-numeric features are skipped entirely rather than coerced
into a statistic that wouldn't mean anything.

Unlike `mean()`, the median is not pulled toward an unusually large or small value in the
data — it is the middle observation once the feature's values are sorted (or the average
of the two middle observations, for a feature with an even number of observed values).
That makes it a useful cross-check against `mean()` on a feature you suspect has extreme
values on one side, such as `unit_price` in the example below.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one median per feature. `Axis::Feature` reduces across each row's features
instead, producing one value per case.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that feature's result:

- `true` (the default): missing values are excluded before sorting, so the median is
  computed only from the observed values.
- `false`: a single missing value in a feature poisons that feature's result, which comes
  back `NULL`.

## Returns

`Slate`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's median.

## Examples

Example 1. Compute the median of every numeric feature, skipping missing values (the
default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto middles = orders.median();
std::cout << middles;
```

### Output

```text
  order_id  103.5
     units  2.5
unit_price  320
  discount  0.08
ptype: Series
```

`unit_price`'s median (`320`) sits much closer to the bulk of its values (`45.5`, `42.0`,
`320.0`) than its mean of `551.5` (see [`mean()`](mean.md)'s Example 1) does — the two
large values `1200.0` and `1150.0` pull the mean upward but only shift the median by one
sorted position. `units` has an even count of observed values (`6`), so its median
(`2.5`) is the average of the two middle sorted values (`2` and `3`), not one of the
original observations.

Example 2. Disable null-skipping to see a missing value propagate.

```cpp
auto middles_strict = orders.median(Axis::Case, false);
std::cout << middles_strict;
```

### Output

```text
  order_id  103.5
     units  2.5
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, `unit_price` and `discount` — the two features with a missing
value — come back `NULL` entirely rather than a median computed from a reduced set of
observations. `order_id` and `units`, with no missing values, are unaffected either way.

## Notes

- `median()`'s result is labeled by feature name and prints like a small labeled
  `Series`, not like a row sliced from the source frame.
- Non-numeric features never appear in the result; they are neither sorted as an error
  nor silently coerced.
- For a feature with an even number of observed values, the median is the arithmetic
  average of the two central sorted values — it will not necessarily equal any actual
  observation in the data, the same way `units`' median of `2.5` does not appear in its
  source values `{2, 5, 1, 3, 4, 2}`.
- `median()` is a special case of [`percentiles()`](percentiles.md) at the `0.5` point;
  reach for `percentiles()` directly when you also need the `25%`/`75%` quartiles in the
  same call.

## See Also

- [`DataFrame::mean`](mean.md): the corresponding average, more sensitive to extreme
  values than the median.
- [`DataFrame::percentiles`](percentiles.md): the median generalized to any quantile, not
  just the midpoint.
- [`DataFrame::describe`](describe.md): reports the median as its `50%` row alongside
  several other statistics in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
