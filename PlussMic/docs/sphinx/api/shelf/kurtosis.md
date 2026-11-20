<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ace325f14e6a -->

# Shelf::kurtosis

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
ShelfValue Shelf::kurtosis(std::int64_t min_count = 4) const
```

## What It Does

Returns the sample excess kurtosis (bias-corrected) of every observed value in
a numeric `Series` shelf — a measure of tail weight relative to a normal
distribution: `0` for normal-like tails, positive for heavier ("fat") tails,
negative for lighter tails. Rather than throwing on a too-small sample, this
method returns a result marked *invalid* whenever the observed count is below
`max(4, min_count)` — kurtosis needs at least 4 observations regardless of
what you pass. See [`Shelf::sum`'s note](sum.md) on why you should check
the scalar validity mask rather than trust the printed number for an invalid
result.

If every observed value is identical (zero variance), the result is `0`
exactly, rather than the `NaN`/`Infinity` that dividing by a zero variance
would otherwise produce.

## Parameters

**min_count** (`std::int64_t`, default `4`)
    The smallest observed count still considered valid. The effective floor is
    always `max(4, min_count)` — kurtosis needs at least 4 observations
    regardless of what you pass here.

## Returns

`ShelfValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the sample excess
    kurtosis.

## Examples

Example 1. A sample with a pronounced outlier — heavier tails than normal.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({1.0, 2.0, 2.0, 3.0, 10.0}));
auto out = values.kurtosis();
std::cout << out.to_dbl();
```

### Output

```text
4.27215
```

Example 2. Identical values produce exactly `0`, not `NaN`.

```cpp
auto flat = Shelf(pluss::make_series({5.0, 5.0, 5.0, 5.0}));
auto out2 = flat.kurtosis();
std::cout << out2.to_dbl();
```

### Output

```text
0
```

Example 3. Fewer than 4 observed values marks the result invalid rather than
throwing.

```cpp
auto small = Shelf(pluss::make_series({1.0, 2.0, 3.0}));
auto out3 = small.kurtosis();
std::cout << out3.shelf().notnull().as_series().host<bool>()[0];
```

### Output

```text
0
```

## Notes

- The effective minimum observed count is always `max(4, min_count)` — passing
  a smaller `min_count` doesn't lower the floor below `4`.
- Zero variance (every observed value identical) yields `0` exactly, never
  `NaN`.
- Below the effective minimum count, the result is marked invalid rather than
  thrown — see the caveat on [`Shelf::sum`'s page](sum.md) about checking
  the scalar validity mask rather than trusting the printed number.
- Restricted to numeric `Series` payloads; anything else throws
  `"Shelf::kurtosis: expected a real numeric Series Shelf"`.

## See Also

- [`Shelf::skew`](skew.md): the third-moment counterpart, measuring
  asymmetry instead of tail weight.
- [`Shelf::mean`](mean.md) / [`Shelf::var`](var.md): the lower moments
  kurtosis builds on.
- [`Shelf::describe`](describe.md): a broader numeric summary that doesn't
  include kurtosis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
