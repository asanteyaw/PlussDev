<!-- pluss-hand-authored -->

# Series::masked_scatter_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::masked_scatter_(const Series& mask, const Series& src) const
```

## What It Does

`masked_scatter_()` performs the same sequential mask-driven write as
[`masked_scatter()`](masked_scatter.md), but it writes directly into the
current `Series` and returns the same handle.

Use it when the masked write itself is the state change you want to keep.

## Parameters

The parameters are the same as [`masked_scatter()`](masked_scatter.md):

- `mask` must be boolean and may broadcast to the current destination shape
- `src.numel()` must equal the number of `true` positions in the broadcasted
  mask

## Returns

`Series&`
    The same `Series`, after mutation.

## Examples

Example 1. A basic in-place masked scatter updates only the `true` positions.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto mask = pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool);
auto src = pluss::make_series({1.0, 2.0});

base.masked_scatter_(mask, src);
std::cout << base;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 20, 2, 40]
```

Example 2. Broadcasted masks work on the in-place form too.

```cpp
auto matrix = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
});
auto column_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto src = pluss::make_series({1.0, 2.0, 3.0, 4.0});

matrix.masked_scatter_(column_mask, src);
std::cout << matrix;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 20, 2],
  [3, 50, 4]]
```

## Gotchas And Failure Modes

- This method mutates the current value.
- Source values are consumed sequentially in logical mask order.
- The same boolean-mask and source-count rules from [`masked_scatter()`](masked_scatter.md)
  still apply.

## Summary

Use `masked_scatter_()` when boolean-mask-driven writes should land directly on
the current `Series`.

## See Also

- [`Series::masked_scatter`](masked_scatter.md): the out-of-place sibling.
- [`Series::scatter_`](scatter_.md): in-place indexed writes by integer
  positions instead of a mask.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="masked_scatter.html">
    <span class="label">Previous</span>
    <span class="title">masked_scatter</span>
  </a>
  <a class="next-link" href="masked_select.html">
    <span class="label">Next</span>
    <span class="title">masked_select</span>
  </a>
</div>
