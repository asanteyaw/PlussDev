<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 514fa7c08ea4 -->

# DataFrame::se

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::se(Axis axis = Axis::Case, ErrorTarget target = ErrorTarget::Mean, const StatisticOptions& options = {}) const -> Slate
```

## What It Does

Reduces every numeric feature to the standard error of its mean (SEM) — the sample
standard deviation divided by the square root of the observation count,
`sqrt(variance / n)`. This measures how precisely each feature's sample mean
estimates the true population mean; a smaller `se()` means the mean is measured with
more precision (either lower spread, more observations, or both).

Only `ErrorTarget::Mean` is currently supported — `target` exists as a forward-looking
parameter, and passing anything else throws `"DataFrame::se: only ErrorTarget::Mean is
currently supported"`.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Reduction direction. `Axis::Case` (the default) reduces down each feature's cases,
producing one standard error per feature. `Axis::Feature` reduces across each row's
features instead.

**target** (`ErrorTarget`, default `ErrorTarget::Mean`)

Which statistic's standard error to compute. Only `ErrorTarget::Mean` is currently
implemented.

**options** (`const StatisticOptions&`, default `{}`)

The same structured policy [`var()`](var.md) and [`std()`](std.md) accept:

- `ddof` (`std::int64_t`, default `1`): delta-degrees-of-freedom subtracted in the
  underlying variance's divisor — `ddof = 1` is the sample variance feeding this
  standard error.
- `min_count` (`std::int64_t`, default `1`): the minimum number of non-null
  observations a feature needs before it gets a real result rather than `NULL`.

## Returns

`Slate`
    One entry per participating numeric feature, labeled by feature name, holding
    that feature's standard error of the mean.

## Examples

Example 1. Compute the standard error of the mean for each numeric feature.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
    {"discount",   Slate(pluss::make_series({0.1, 0.05, std::numeric_limits<double>::quiet_NaN(), 0.08, 0.05, 0.12}))},
});

auto standard_errors = orders.se();
std::cout << standard_errors;
```

### Output

```text
  order_id  0.7637626158
     units  0.6009252126
unit_price  259.6124997
  discount  0.01378404875
ptype: Series
```

Each entry is `sqrt(variance / n)` using [`var()`](var.md)'s own results on this
identical dataset: `order_id`'s variance is `3.5` over `6` observations, so its
standard error is `sqrt(3.5 / 6) ≈ 0.7638`. `unit_price` and `discount` each have one
missing observation (case `5`), so their standard errors divide by `5`, not `6` —
`sqrt(336993.25 / 5) ≈ 259.61`.

## Notes

- `se()`'s result is labeled by feature name and prints like a small labeled
  `Series`, the same shape as [`var()`](var.md) and [`std()`](std.md)'s results.
- Non-numeric features never appear in the result.
- A feature with fewer than two observed values has an undefined sample variance
  (and therefore an undefined standard error) and reports `NULL`, independent of
  `min_count`.
- Only `ErrorTarget::Mean` is currently supported; passing any other target throws.

## See Also

- [`DataFrame::var`](var.md), [`DataFrame::std`](std.md): the variance and standard
  deviation this standard error is derived from.
- [`DataFrame::mean`](mean.md): the statistic whose precision `se()` measures.
- [`DataFrame::describe`](describe.md): a broader statistical summary that does not
  include the standard error directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scan.html">
    <span class="label">Previous</span>
    <span class="title">scan</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
