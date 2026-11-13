<!-- pluss-hand-authored -->

# Series::invalid

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::invalid() const
```

## What It Does

`invalid()` returns a boolean mask that is true exactly where a lane is not a
usable finite numeric value.

On the current public surface that means:

- floating lanes become true when they are `nan`, `+inf`, or `-inf`
- complex lanes become true when either the real part or the imaginary part is
  non-finite
- integral and boolean lanes stay false, because those dtypes do not carry
  `nan` or infinity states

Shape is preserved. `invalid()` does not remove values or repair them; it only
classifies them.

## Parameters

None.

## Returns

`Series`
    A boolean `Series` with the same shape as `self`.

## Gradient

`invalid()` is a diagnostic mask, not a differentiable numeric transform.

## Examples

Example 1. A mixed matrix of ordinary values, `nan`, and infinities turns into
an equally shaped boolean mask.

```cpp
auto values = pluss::make_series({
    {1.0, std::numeric_limits<double>::quiet_NaN(), 2.0},
    {std::numeric_limits<double>::infinity(),
     -std::numeric_limits<double>::infinity(),
     5.0},
});

pluss::series::print(std::cout, values.invalid());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false true false ⎤
⎣  true true false ⎦
```

Example 2. Integral input has no `nan` or infinity lanes, so the whole mask is
false.

```cpp
std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).invalid();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, false, false]
```

## Gotchas And Failure Modes

- `invalid()` preserves shape; it is for marking bad lanes, not extracting them.
- If the next step is repair, pair this with [`nan_to_num()`](nan_to_num.md) or
  a branch like [`where()`](where.md).
- If the next step is compaction into positions, use
  [`invalid_index()`](invalid_index.md).

## Summary

Use `invalid()` when the main question is "where are the bad numeric lanes?"
rather than "what should I replace them with?"

## See Also

- [`Series::invalid_index`](invalid_index.md): compact the same diagnosis into
  flat positions.
- [`Series::isfinite`](isfinite.md): broader finite-value detector with the
  same shape-preserving mask style.
- [`Series::nan_to_num`](nan_to_num.md): replace invalid lanes with explicit
  policy values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersection_along.html">
    <span class="label">Previous</span>
    <span class="title">intersection_along</span>
  </a>
  <a class="next-link" href="invalid_index.html">
    <span class="label">Next</span>
    <span class="title">invalid_index</span>
  </a>
</div>
