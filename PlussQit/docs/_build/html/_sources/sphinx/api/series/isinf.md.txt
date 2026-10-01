<!-- pluss-hand-authored -->

# Series::isinf

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::isinf() const -> Series
Series& Series::isinf_()
```

## What It Does

`isinf()` returns a boolean mask that is true exactly where the value is either
positive infinity or negative infinity.

This is narrower than [`isfinite()`](isfinite.md) and different from
[`isnan()`](isnan.md):

- infinities are matched here
- `nan` is not matched here
- finite numbers are not matched here

## Returns

`Series`
    A boolean `Series` with the same shape as `self`.

## Gradient

`isinf()` is a boolean detector, not a differentiable numeric transform.

## In-Place Form

`isinf_()` is the in-place sibling of [`isinf()`](isinf.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()});
x.isinf_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1]
```

## Examples

Example 1. Both `+inf` and `-inf` are marked as true.

```cpp
auto special = pluss::make_series({
    -std::numeric_limits<double>::infinity(),
    -1.0,
    std::numeric_limits<double>::quiet_NaN(),
    0.0,
    std::numeric_limits<double>::infinity(),
});

std::cout << special.isinf();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=bool, platform=cpu, numel=5, contiguous=true)
data=[true, false, false, false, true]
```

## Gotchas And Failure Modes

- `nan` does not count as infinity.
- The output is a mask, not a repaired numeric result.
- Use this when overflow-like conditions matter specifically, not when you want
  the broader non-finite test.

## Summary

Use `isinf()` to isolate values that have blown out to positive or negative
infinity.

## See Also

- [`Series::isfinite`](isfinite.md): finite-value detector.
- [`Series::isnan`](isnan.md): `nan` detector.
- [`Series::nan_to_num`](nan_to_num.md): replace infinities after detection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isin.html">
    <span class="label">Previous</span>
    <span class="title">isin</span>
  </a>
  <a class="next-link" href="isnan.html">
    <span class="label">Next</span>
    <span class="title">isnan</span>
  </a>
</div>
