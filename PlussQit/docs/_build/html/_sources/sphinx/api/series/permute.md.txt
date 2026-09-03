<!-- pluss-hand-authored -->

# Series::permute

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::permute(pluss::RangeView<std::int64_t> dims) const -> Series
```

## What It Does

Reorders all axes at once according to `dims` and returns the result as a view.

Where [`transpose()`](transpose.md) swaps one pair of axes, `permute()` lets you
state the entire destination order in a single call. The storage is still
shared; only shape and stride interpretation change, so the result is usually
non-contiguous.

This is the right method when the intent is "move the last axis to the front",
"make channels first", or "rotate the whole rank", not just swap one pair.

## Parameters

**dims** (`pluss::RangeView<std::int64_t>`)

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

The examples on this page build incrementally.

Example 1. Reorder a `2 x 3 x 4` series into `[4, 2, 3]`.

```cpp
auto cube = pluss::make_series(
    {
        {
            {1.0, 2.0, 3.0, 4.0},
            {5.0, 6.0, 7.0, 8.0},
            {9.0, 10.0, 11.0, 12.0},
        },
        {
            {13.0, 14.0, 15.0, 16.0},
            {17.0, 18.0, 19.0, 20.0},
            {21.0, 22.0, 23.0, 24.0},
        },
    });
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

Example 2. Negative dimensions are normalized from the back before the
permutation is checked.

```cpp
std::cout << cube.permute({-1, 0, 1});
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

`{-1, 0, 1}` normalizes to `{2, 0, 1}`, so it produces the same result as
Example 1.

Example 3. A duplicate axis is rejected.

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
- Duplicate axes and rank-mismatched axis lists are invalid.
- If all you need is one pairwise swap, [`transpose()`](transpose.md) is
  simpler and clearer.

## See Also

- [`Series::transpose`](transpose.md): the explicit two-axis special case.
- [`Series::unsqueeze`](unsqueeze.md): insert a new axis before permuting if you
  need to align rank first.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="norm.html">
    <span class="label">Previous</span>
    <span class="title">norm</span>
  </a>
  <a class="next-link" href="pile.html">
    <span class="label">Next</span>
    <span class="title">pile</span>
  </a>
</div>
