<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b3318269f2c2 -->

# Shelf::covar

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
ShelfValue Shelf::covar(const Shelf& other, const StatisticOptions& options = {}) const
```

## What It Does

Computes the covariance between this shelf and `other`, as a scalar
`ShelfValue` — the unnormalized counterpart to [`corr()`](corr.md) (`corr()`
is covariance divided by the product of the two standard deviations). Both
shelves must be numeric `Series`. If either shelf carries explicit case
labels, the two are aligned by matching label first — see
[`corr()`'s important callout](corr.md) on how much this can change the
result. `ddof` (in `options`) controls the divisor, exactly like
[`var()`](var.md)'s `StatisticOptions` overload: `ddof = 1` (the default)
gives the sample covariance (`n - 1` divisor); `ddof = 0` gives the
population covariance (`n` divisor). Below `min_count` valid pairs, the result
is marked *invalid* rather than thrown — see
[`Shelf::sum`'s note](sum.md) on checking the scalar validity mask rather than
trusting the printed number in that case.

Unlike `corr()`, this method requires both shelves to already share the same
platform (CPU/CUDA/MPS) — there's no automatic host-side conversion here.

## Parameters

**other** (`const Shelf&`)
    The numeric `Series` shelf to compute covariance with.

**options** (`StatisticOptions`, default `{}`)
    `ddof` (default `1`): the divisor is `n - ddof` over the valid pairs.
    `min_count` (default `1`): the smallest number of valid pairs still
    considered valid; below it (and always below `ddof + 1`), the result is
    marked invalid.

## Returns

`ShelfValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the covariance.

## Examples

Example 1. Sample covariance (the default) between two related shelves.

```cpp
using namespace pluss::table;

auto scores = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Shelf(pluss::make_series({10.0, 10.0, 35.0, 45.0}));
auto out = scores.covar(other);
std::cout << out.to_dbl();
```

### Output

```text
216.667
```

Example 2. Population covariance — `ddof = 0` instead of the default `1`.

```cpp
auto out2 = scores.covar(other, StatisticOptions{.ddof = 0});
std::cout << out2.to_dbl();
```

### Output

```text
162.5
```

Example 3. Below `min_count` valid pairs, the result is marked invalid rather
than thrown.

```cpp
auto small = Shelf(pluss::make_series({1.0, 2.0}));
auto small_other = Shelf(pluss::make_series({3.0, 4.0}));
auto out3 = small.covar(small_other, StatisticOptions{.ddof = 1, .min_count = 5});
std::cout << out3.shelf().notnull().as_series().host<bool>()[0];
```

### Output

```text
0
```

## Notes

- If either shelf carries explicit case labels, pairing is by matching
  label — see [`corr()`'s callout](corr.md).
- `ddof = 1` (the default) gives the sample covariance; `ddof = 0` gives the
  population covariance.
- Below `min_count` valid pairs, the result is marked invalid rather than
  thrown.
- Both shelves must already share the same platform, or it throws
  `"Shelf::covar: operands must share a platform"` — unlike `corr()`, this
  method doesn't convert to a common host representation first.
- Restricted to numeric `Series` payloads; anything else throws
  `"Shelf::covar: expected a real numeric Series Shelf"`.

## See Also

- [`Shelf::corr`](corr.md): the normalized counterpart — a value in `[-1,
  1]` instead of covariance's unbounded scale.
- [`Shelf::var`](var.md): the special case of `covar()` with `other` equal to
  the shelf itself.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="csmax.html">
    <span class="label">Next</span>
    <span class="title">csmax</span>
  </a>
</div>
