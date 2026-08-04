<!-- pluss-hand-authored -->

# Series::permute

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::permute(pluss::msharp::RangeView::Int dims) const -> Series
```

## What It Does

Reorders all axes at once according to `dims` and returns the result as a view.

Where [`transpose()`](transpose.md) swaps exactly two axes, `permute()` lets you
state the entire destination axis order in one shot. The underlying storage is
still shared; only shape and stride interpretation change, so the result is
usually non-contiguous.

This is the method you reach for when the intent is "make channels first",
"bring time to the front", or "rotate the whole axis order", not just swap one
pair of axes.

## Parameters

**dims** (`pluss::msharp::RangeView::Int`)

The destination axis order.

`dims` must:

- have length exactly equal to `self.dim()`
- mention every axis exactly once
- contain only in-range axis indices after negative-dimension normalization

Negative indices are normalized from the end, exactly like
[`transpose()`](transpose.md).

## Returns

`Series`
    A view whose axis order matches `dims`.

## Gradient

`permute()` is differentiable. The backward pass applies the inverse
permutation, so each sensitivity value is sent back to the exact source element
that produced it in the forward pass.

Because the inverse permutation is computed structurally from `dims`, there is
no ambiguity: the gradient rule is determined entirely by axis order, not by
values.

## Examples

Example 1. Reorder a `2 x 3 x 4` series into `[4, 2, 3]`.

```cpp
auto cube = pluss::make_series(
    std::vector<double>{
        1,  2,  3,  4,
        5,  6,  7,  8,
        9,  10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24},
    {2, 3, 4});
std::cout << cube.permute({2, 0, 1});
```

### Output

```text
pluss::Series(shape=[4,2,3], strides=[1,12,4], dtype=float64, platform=cpu, numel=24, contiguous=false)
data=[[[1, 5, 9],
  [13, 17, 21]],
  [[2, 6, 10],
  [14, 18, 22]],
  [[3, 7, 11],
  [15, 19, 23]],
  [[4, 8, 12],
  [16, 20, 24]]]
```

Axis `2` becomes axis `0`, axis `0` becomes axis `1`, and axis `1` becomes
axis `2`. The result is a view, so it preserves the original storage and
becomes non-contiguous.

Example 2. A duplicate axis is rejected.

```cpp
try {
  std::cout << cube.permute({0, 0, 1});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
permute: duplicate dim
```

This is an important contract difference from APIs that try to "best effort"
rearrange shapes: `permute()` requires a real permutation, not just any list of
integers.

## Notes

- `permute()` always describes the full destination order.
- The result is typically non-contiguous because no physical reorder is
  performed.
- If all you need is one pairwise swap, [`transpose()`](transpose.md) is
  simpler and clearer.

## See Also

- [`Series::transpose`](transpose.md): the explicit two-axis special case.
- [`Series::unsqueeze`](unsqueeze.md): insert a new axis before permuting if you
  need to align rank first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html">
    <span class="label">Previous</span>
    <span class="title">transpose</span>
  </a>
  <a class="next-link" href="unsqueeze.html">
    <span class="label">Next</span>
    <span class="title">unsqueeze</span>
  </a>
</div>
