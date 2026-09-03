<!-- pluss-hand-authored -->

# Series::expand

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::expand(pluss::RangeView::Int shape) const -> Series
```

## What It Does

`expand()` returns a broadcast view of the series at a new `shape`, without
copying data. It is the same operation as [`broadcast_to()`](broadcast_to.md),
with one extra convenience: an entry of `-1` in `shape` means "keep this axis at
its current size".

The rules:

- `shape` is matched from the right; extra leading dimensions are added
- an axis of size `1` is stretched to the requested size, and gets a stride of
  `0`
- an axis already at the requested size (or requested as `-1`) is untouched
- a non-size-`1` axis cannot be resized — that is an error

Because the stretched axes have stride `0`, the result is a free, read-only-ish
view sharing storage with the source. Materialize it with
[`contiguous()`](contiguous.md) or [`clone()`](clone.md) when you need a real
array.

## Parameters

**shape** (`pluss::RangeView::Int`)

The target shape. Entries may be `-1` to keep the corresponding current size.
Must have at least the current rank.

## Returns

`Series`
    A broadcast view with stride `0` on the stretched axes. Same dtype and
    platform.

## Gradient

`expand()` is differentiable. The backward sums the upstream gradient over each
stretched (and each added leading) axis and reshapes it back to the original.

## Examples

Example 1. A `3 x 1` column stretched along the second axis.

```cpp
auto col = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);

std::cout << col.expand({3, 4});
```

### Output

```text
pluss::Series(shape=[3,4], strides=[1,0], dtype=float64, platform=cpu, numel=12, contiguous=false)
data=[[1, 1, 1, 1],
  [2, 2, 2, 2],
  [3, 3, 3, 3]]
```

Example 2. `-1` keeps an axis as it is. Here nothing actually expands, so the
result is still the original contiguous `3 x 1`.

```cpp
auto col = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);

std::cout << col.expand({3, -1});
```

### Output

```text
pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [2],
  [3]]
```

Example 3. A non-size-`1` axis cannot be resized.

```cpp
try {
  pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1).expand({2, 4});
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
expand: non-singleton dimension mismatch
```

Example 4. The backward sums over the stretched axis.

```cpp
auto col = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);
col.tracks_slope_(true);

col.expand({3, 4}).sum().reverse();

std::cout << col.sensitivity();
```

### Output

```text
pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[4],
  [4],
  [4]]
```

Each row's single element was replicated four times, so it accumulates `4`.

## Gotchas And Failure Modes

- The result is a stride-`0` view. It is not safe to mutate in place, and it is
  not contiguous — materialize with [`contiguous()`](contiguous.md) first.
- Only size-`1` axes can grow. `expand()` on a non-singleton axis raises.
- `-1` means "keep", not "infer a total" — it never changes an axis.

## Summary

`expand(shape)` is a zero-copy broadcast view, with `-1` meaning "keep this
axis":

- size-`1` axes get stride `0`; result is non-contiguous
- differentiable, backward sums the gradient over the stretched axes

## See Also

- [`Series::broadcast_to`](broadcast_to.md): the identical operation without the
  `-1` shorthand.
- [`Series::expand_as`](expand_as.md): expand to match another series' shape.
- [`Series::repeat`](repeat.md): the copying alternative.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp2.html">
    <span class="label">Previous</span>
    <span class="title">exp2</span>
  </a>
  <a class="next-link" href="expand_as.html">
    <span class="label">Next</span>
    <span class="title">expand_as</span>
  </a>
</div>
