<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 44cdc7566318 -->

# DataFrame::var

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::var(Axis axis = Axis::Case, bool skipnull = true) const -> Slate
DataFrame::var(Axis axis, const StatisticOptions& options) const -> Slate
```

## What It Does

Reduces every numeric feature to its variance, returning one `Slate` whose entries line
up with the frame's numeric feature names — the same shape [`std()`](std.md) returns,
without the final square root. Non-numeric features are skipped entirely rather than
coerced into a statistic that wouldn't mean anything.

By default this is the **sample** variance: the sum of squared deviations from the mean
is divided by `n - 1` rather than `n`. The `StatisticOptions` overload exposes the same
two fields as `std()`'s structured overload — `ddof` to switch to the population
variance, and `min_count` to require a minimum number of observations before a feature's
result counts as valid.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one variance per feature. `Axis::Feature` reduces across each row's features
instead, producing one value per case.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that feature's result:

- `true` (the default): missing values are excluded from the computation entirely, so
  the variance reflects only the observed values.
- `false`: a single missing value in a feature poisons that feature's result, which comes
  back `NULL`.

**options** (`const StatisticOptions&`)

A structured policy for the same reduction:

- `ddof` (`std::int64_t`, default `1`): the delta-degrees-of-freedom subtracted from `n`
  in the divisor. `ddof = 1` (the default under either overload) is the sample variance;
  `ddof = 0` is the population variance, dividing by the full observed count instead of
  `n - 1`.
- `min_count` (`std::int64_t`, default `1`): the minimum number of non-null observations
  a feature must have before it is assigned a real result at all; a feature with fewer
  observed values than `min_count` reports `NULL` regardless of `ddof`.

## Returns

`Slate`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's variance.

## Examples

Example 1. Compute the sample variance, skipping missing values (the default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto spread = orders.var();
std::cout << spread;
```

### Output

```text
  order_id  3.5
     units  2.166666667
unit_price  336993.25
  discount  0.00095
ptype: Series
```

Each entry is exactly the corresponding entry in [`std()`](std.md)'s Example 1, squared —
`336993.25` is `580.5111971^2`, for instance. `unit_price`'s one missing observation
(case `5`) is excluded from the computation, the same null-skipping rule every other
feature reduction in this table applies.

Example 2. Disable null-skipping to see a missing value propagate.

```cpp
auto spread_strict = orders.var(Axis::Case, false);
std::cout << spread_strict;
```

### Output

```text
  order_id  3.5
     units  2.166666667
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, `unit_price` and `discount` come back `NULL` entirely, exactly
mirroring [`std()`](std.md)'s Example 2.

Example 3. Use the `StatisticOptions` overload to compute the population variance
instead of the sample variance.

```cpp
StatisticOptions population;
population.ddof = 0;

auto spread_population = orders.var(Axis::Case, population);
std::cout << spread_population;
```

### Output

```text
  order_id  2.916666667
     units  1.805555556
unit_price  269594.6
  discount  0.00076
ptype: Series
```

Every value is smaller than in Example 1, and each is the corresponding population
`std()` value from that method's own Example 3, squared. `ddof = 0` divides by the full
observed count rather than `count - 1`; use it when the data in hand is the entire
population of interest rather than a sample.

## Notes

- `var()`'s result is labeled by feature name and prints like a small labeled `Series`,
  not like a row sliced from the source frame.
- Non-numeric features never appear in the result; they are neither computed as an error
  nor silently coerced.
- A feature with fewer than two observed values has an undefined sample variance and
  reports `NULL`, independent of `min_count`.
- `var()` and `std()` always agree up to a square root: `std()`'s result is `var()`'s
  result with `std::sqrt` applied element-wise, under matching `ddof`.

## See Also

- [`DataFrame::std`](std.md): the square root of this statistic, in the same units as the
  original data rather than squared units.
- [`DataFrame::mean`](mean.md): the center each squared deviation in this computation is
  measured from.
- [`DataFrame::describe`](describe.md): reports `std` (not `var` directly) alongside
  several other statistics in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="update.html">
    <span class="label">Previous</span>
    <span class="title">update</span>
  </a>
  <a class="next-link" href="view.html">
    <span class="label">Next</span>
    <span class="title">view</span>
  </a>
</div>
