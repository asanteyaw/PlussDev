<!-- pluss-hand-authored -->

# Series::nan_to_num

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::nan_to_num(double nan, double posinf, double neginf) const -> Series
```

## What It Does

`nan_to_num()` repairs numerically problematic lanes by replacing `nan`,
positive infinity, and negative infinity with ordinary numeric fallbacks that
you choose explicitly.

Finite values pass through unchanged. Only the exceptional lanes are rewritten.

This is one of the most practical cleanup surfaces in `Series` because it lets
you say, in one call, what should happen to each problematic numeric state.

## Parameters

**nan** (`double`)

Replacement value for every `nan` lane.

**posinf** (`double`)

Replacement value for every positive-infinity lane.

**neginf** (`double`)

Replacement value for every negative-infinity lane.

The three toggles are independent. That matters because many projects want
different semantics for "missing", "overflowed positively", and "overflowed
negatively".

## Returns

`Series`
    A new `Series` with the same shape as `self`, but with exceptional numeric
    states repaired according to the three replacement parameters.

## Examples

Example 1. Each problematic numeric state can be mapped to its own explicit
replacement.

```cpp
auto special = pluss::make_series({
    -std::numeric_limits<double>::infinity(),
    -1.0,
    std::numeric_limits<double>::quiet_NaN(),
    0.0,
    std::numeric_limits<double>::infinity(),
});

std::cout << special.nan_to_num(0.0, 99.0, -99.0);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-99, -1, 0, 0, 99]
```

The replacement map in that example is:

- `nan -> 0`
- `+inf -> 99`
- `-inf -> -99`

Finite lanes like `-1` and `0` survive untouched.

Example 2. The same repair logic applies elementwise across matrices.

```cpp
auto messy = pluss::make_series({
    {std::numeric_limits<double>::quiet_NaN(), 1.5, std::numeric_limits<double>::infinity()},
    {-std::numeric_limits<double>::infinity(), -2.5, 0.0},
});

pluss::series::print(std::cout, messy.nan_to_num(-7.0, 500.0, -500.0));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-7, 1.5, 500],
  [-500, -2.5, 0]]
```

## Gotchas And Failure Modes

- `nan_to_num()` does not touch already-finite values.
- The three replacement parameters are positional, so keep their roles straight:
  `nan`, then `posinf`, then `neginf`.
- This is a cleanup transform, not a diagnostic mask. If you need detection
  first, use `isnan()`, `isinf()`, or `isfinite()`.

## Summary

Use `nan_to_num()` when unstable numeric states should be turned back into
explicit finite policy values before the next stage of computation.

## See Also

- [`Series::isnan`](isnan.md): detect `nan` before replacing it.
- [`Series::isinf`](isinf.md): detect infinities specifically.
- [`Series::isfinite`](isfinite.md): broad finite/non-finite detector.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mvlgamma.html">
    <span class="label">Previous</span>
    <span class="title">mvlgamma</span>
  </a>
  <a class="next-link" href="nanmean.html">
    <span class="label">Next</span>
    <span class="title">nanmean</span>
  </a>
</div>
