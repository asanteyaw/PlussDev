<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 98bb8dab3bea -->

# Slate::se

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::se(ErrorTarget target = ErrorTarget::Mean, const StatisticOptions& options = {}) const -> SlateValue
```

## What It Does

Returns the standard error of the mean — `sqrt(variance / count)`, using the
same `ddof`-adjusted variance [`var(options)`](var.md) computes. `target`
exists to select *what* the standard error is computed for, but only
`ErrorTarget::Mean` is currently implemented; passing `ErrorTarget::Median`
throws rather than silently computing something else. Like the
`StatisticOptions` overload of `var()`, this method returns a result marked
*invalid* (rather than throwing) when the observed count is below
`min_count` — see [`Slate::sum`'s note](sum.md) on why you should check
the scalar validity mask rather than trust the printed number in that case.

## Parameters

**target** (`ErrorTarget`, default `Mean`)
    Which statistic's standard error to compute. Only `Mean` is currently
    implemented; `Median` throws.

**options** (`StatisticOptions`, default `{}`)
    `ddof` (default `1`): the same divisor adjustment `var()`/`std()` use.
    `min_count` (default `1`): the smallest observed count still considered
    valid; below it, the result is marked invalid rather than thrown.

## Returns

`SlateValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the standard error of
    the mean.

## Examples

Example 1. Standard error of the mean, default options.

```cpp
using namespace pluss::table;

auto values = Slate(pluss::make_series({2.0, 4.0, 6.0, 8.0}));
auto out = values.se();
std::cout << out.to_dbl();
```

### Output

```text
1.29099
```

This is `sqrt(var() / count)` — `sqrt(6.66667 / 4)`.

Example 2. `ErrorTarget::Median` throws — it isn't implemented.

```cpp
try {
  auto bad = values.se(ErrorTarget::Median);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::se: only ErrorTarget::Mean is implemented in this pass
```

Example 3. Below `min_count`, the result is marked invalid rather than
thrown.

```cpp
auto small = Slate(pluss::make_series({10.0, 20.0}));
auto out3 = small.se(ErrorTarget::Mean, StatisticOptions{.ddof = 1, .min_count = 5});
std::cout << out3.slate().notnull().as_series().host<bool>()[0];
```

### Output

```text
false
```

## Notes

- Only `ErrorTarget::Mean` is implemented; `ErrorTarget::Median` throws
  `"Slate::se: only ErrorTarget::Mean is implemented in this pass"`.
- Below `min_count`, the result is marked invalid rather than thrown — see the
  important caveat on [`Slate::sum`'s page](sum.md) about checking
  the scalar validity mask rather than trusting the printed number.
- Restricted to numeric `Series` payloads; anything else throws
  `"Slate::se: expected a real numeric Series Slate"`.

## See Also

- [`Slate::var`](var.md) / [`Slate::std`](std.md): the variance and standard
  deviation this method's standard error is derived from.
- [`Slate::mean`](mean.md): the statistic whose standard error this method
  reports.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scan.html">
    <span class="label">Previous</span>
    <span class="title">scan</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
