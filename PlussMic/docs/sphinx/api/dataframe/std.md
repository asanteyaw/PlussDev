<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8a18966402e1 -->

# DataFrame::std

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
Slate DataFrame::std(Axis axis = Axis::Case, bool skipnull = true) const
Slate DataFrame::std(Axis axis, const StatisticOptions& options) const
```

## What It Does

Reduces every numeric feature to its standard deviation, returning one `Slate` whose
entries line up with the frame's numeric feature names — the same shape
[`sum()`](sum.md) and [`mean()`](mean.md) return. Non-numeric features are skipped
entirely rather than coerced into a statistic that wouldn't mean anything.

By default this is the **sample** standard deviation: the sum of squared deviations from
the mean is divided by `n - 1` rather than `n`, the conventional correction for
estimating a population's spread from a finite sample. The `StatisticOptions` overload
lets you override that divisor explicitly via `ddof` when you need the **population**
standard deviation instead (divide by `n`), or require a minimum number of observations
before a feature's result counts as valid.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one standard deviation per feature. `Axis::Feature` reduces across each row's
features instead, producing one value per case.

**skipnull** (`bool`, default `true`)

Controls how a missing value affects that feature's result:

- `true` (the default): missing values are excluded from the computation entirely, so
  the standard deviation reflects only the observed values.
- `false`: a single missing value in a feature poisons that feature's result, which comes
  back `NULL`.

**options** (`const StatisticOptions&`)

A structured policy for the same reduction, exposing two fields the short overload
cannot reach:

- `ddof` (`std::int64_t`, default `1`): the delta-degrees-of-freedom subtracted from `n`
  in the divisor. `ddof = 1` (the default under either overload) is the sample standard
  deviation; `ddof = 0` is the population standard deviation, dividing by the full
  observed count instead of `n - 1`.
- `min_count` (`std::int64_t`, default `1`): the minimum number of non-null observations
  a feature must have before it is assigned a real result at all; a feature with fewer
  observed values than `min_count` reports `NULL` regardless of `ddof`.

## Returns

`Slate`
    One entry per participating numeric feature, labeled by feature name, holding that
    feature's standard deviation.

## Examples

Example 1. Compute the sample standard deviation, skipping missing values (the default).

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto spread = orders.std();
std::cout << spread;
```

### Output

```text
  order_id  1.870828693
     units  1.471960144
unit_price  580.5111971
  discount  0.03082207001
ptype: Series
```

`unit_price`'s one missing observation (case `5`) is excluded from the computation
entirely — its standard deviation is computed over the remaining five values, the same
way [`mean()`](mean.md) and [`describe()`](describe.md) treat that column's missing
value.

Example 2. Disable null-skipping to see a missing value propagate.

```cpp
auto spread_strict = orders.std(Axis::Case, false);
std::cout << spread_strict;
```

### Output

```text
  order_id  1.870828693
     units  1.471960144
unit_price  NULL
  discount  NULL
ptype: Series
```

With `skipnull = false`, `unit_price` and `discount` — the two features that each have
exactly one missing value — come back `NULL` entirely. `order_id` and `units`, with no
missing values, are unaffected either way.

Example 3. Use the `StatisticOptions` overload to compute the population standard
deviation instead of the sample standard deviation.

```cpp
StatisticOptions population;
population.ddof = 0;

auto spread_population = orders.std(Axis::Case, population);
std::cout << spread_population;
```

### Output

```text
  order_id  1.707825128
     units  1.343709625
unit_price  519.2249994
  discount  0.0275680975
ptype: Series
```

Every value is slightly smaller than in Example 1: dividing by the full observed count
(`ddof = 0`) instead of `count - 1` (`ddof = 1`, the implicit default) always produces a
standard deviation that is less than or equal to the sample statistic. Reach for
`ddof = 0` when the data in hand **is** the entire population of interest, not a sample
drawn from a larger one.

## Notes

- `std()`'s result is labeled by feature name and prints like a small labeled `Series`,
  not like a row sliced from the source frame.
- Non-numeric features never appear in the result; they are neither computed as an error
  nor silently coerced.
- A feature with fewer than two observed values has an undefined sample standard
  deviation and reports `NULL`, independent of `min_count`.
- For the corresponding variance (the squared statistic, no square root applied), see
  [`var()`](var.md), which shares the exact same `axis` / `skipnull` / `StatisticOptions`
  shape.

## See Also

- [`DataFrame::var`](var.md): the variance this method's square root is computed from.
- [`DataFrame::mean`](mean.md): the center each deviation in this computation is measured
  from.
- [`DataFrame::describe`](describe.md): reports `std` alongside several other statistics
  in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stack.html">
    <span class="label">Previous</span>
    <span class="title">stack</span>
  </a>
  <a class="next-link" href="sub.html">
    <span class="label">Next</span>
    <span class="title">sub</span>
  </a>
</div>
