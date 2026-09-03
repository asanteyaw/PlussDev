<!-- pluss-hand-authored -->

# Series::isnan

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::isnan() const -> Series
```

## What It Does

`isnan()` returns a boolean mask that is true exactly where a lane contains
`nan`.

This is the sharpest detector in the numeric-state family: it does not match
finite numbers, and it does not match infinities.

## Returns

`Series`
    A boolean `Series` with the same shape as `self`.

## Gradient

`isnan()` is a boolean classification surface, not a differentiable numeric
transform.

## Examples

Example 1. Only the `nan` lane is marked as true.

```cpp
auto special = pluss::make_series({
    -std::numeric_limits<double>::infinity(),
    -1.0,
    std::numeric_limits<double>::quiet_NaN(),
    0.0,
    std::numeric_limits<double>::infinity(),
});

std::cout << special.isnan();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=bool, platform=cpu, numel=5, contiguous=true)
data=[false, false, true, false, false]
```

## Gotchas And Failure Modes

- `isnan()` does not catch infinities.
- Use it when missing or undefined numeric state is the concern specifically.
- If the next step is cleanup, pair it with [`nan_to_num()`](nan_to_num.md).

## Summary

Use `isnan()` when the question is specifically whether a value is undefined as
`nan`.

## See Also

- [`Series::isinf`](isinf.md): catches infinities instead.
- [`Series::isfinite`](isfinite.md): broad finite/non-finite mask.
- [`Series::nan_to_num`](nan_to_num.md): replace `nan` and infinities with
  explicit numeric substitutes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isinf.html">
    <span class="label">Previous</span>
    <span class="title">isinf</span>
  </a>
  <a class="next-link" href="le.html">
    <span class="label">Next</span>
    <span class="title">le</span>
  </a>
</div>
