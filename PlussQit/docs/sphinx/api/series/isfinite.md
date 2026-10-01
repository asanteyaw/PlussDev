<!-- pluss-hand-authored -->

# Series::isfinite

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::isfinite() const -> Series
Series& Series::isfinite_()
```

## What It Does

`isfinite()` classifies each element and returns a boolean mask that is true
exactly where the value is an ordinary finite number.

That means:

- ordinary integers and finite floating values become `true`
- `nan` becomes `false`
- positive infinity becomes `false`
- negative infinity becomes `false`

This method is often the cleanest first diagnostic when a pipeline starts
showing unstable numeric output.

## Returns

`Series`
    A boolean `Series` with the same shape as `self`.

## Gradient

`isfinite()` is a boolean classification surface, not a differentiable numeric
transform.

## In-Place Form

`isfinite_()` is the in-place sibling of [`isfinite()`](isfinite.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()});
x.isfinite_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
```

## Examples

Example 1. Finite values pass; `nan` and both infinities fail.

```cpp
auto special = pluss::make_series({
    -std::numeric_limits<double>::infinity(),
    -1.0,
    std::numeric_limits<double>::quiet_NaN(),
    0.0,
    std::numeric_limits<double>::infinity(),
});

pluss::series::print(std::cout, special.isfinite());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=bool, platform=cpu, numel=5, contiguous=true)
data=[false, true, false, true, false]
```

## Gotchas And Failure Modes

- `isfinite()` is broader than `!isnan()`: infinities also fail.
- The output is boolean control data, not cleaned numeric data.
- If the next step is repair rather than diagnosis, pair this with
  [`nan_to_num()`](nan_to_num.md).

## Summary

Use `isfinite()` to ask the most general stability question: "Which entries are
ordinary finite numbers?"

## See Also

- [`Series::isnan`](isnan.md): detect `nan` only.
- [`Series::isinf`](isinf.md): detect infinities only.
- [`Series::nan_to_num`](nan_to_num.md): replace problematic values after you
  detect them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isclose.html">
    <span class="label">Previous</span>
    <span class="title">isclose</span>
  </a>
  <a class="next-link" href="isin.html">
    <span class="label">Next</span>
    <span class="title">isin</span>
  </a>
</div>
