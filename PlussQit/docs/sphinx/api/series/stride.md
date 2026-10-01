<!-- pluss-hand-authored -->

# Series::stride

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::int64_t Series::stride(std::int64_t dim) const
```

## What It Does

Reports how many storage elements the address advances when the coordinate
on one axis increases by one.

`stride(dim)` answers that for a single axis; [`strides()`](strides.md) returns
the complete axis-by-axis sequence at once. Strides are measured in **elements**,
not bytes — multiply by [`dtype`](dtype.md)`().itemsize` to get a byte
offset.

Stride is the layer beneath [`size`](size.md): `size(dim)` says how many
coordinates axis `dim` has, `stride(dim)` says how far apart, in storage,
consecutive coordinates on that axis are. Together they are the addressing
formula for every element: the storage offset of coordinate
`(i0, i1, ..., ik)` is `sum(ij * stride(j))` plus the tensor's base offset.
Most `Series` code never needs that formula directly — it matters when you
are reasoning about whether a view is [`contiguous`](contiguous.md), whether
two views alias the same memory, or why an in-place op rejects a particular
shape.

For a contiguous `[2, 3]` matrix, moving one column advances one element
while moving one row skips three — strides `[3, 1]`. That is the *default*
layout a fresh tensor gets; nothing about `stride()` itself assumes it.
Transposing, slicing with a step, or broadcasting all change strides without
moving a single value.

Negative axes normalize the usual way: `-1` means the final axis, `-dim()`
the first. Values outside `[-dim(), dim() - 1]` raise.

## Parameters

**dim** (`std::int64_t`): the axis to inspect. Negative values count from the
end.

## Returns

`std::int64_t`
    The stride of axis `dim`, in elements. `strides()` returns the same
    information for every axis at once, in axis order.

## Gradient

Not applicable — both return plain integers, not a `Series`.

## Examples

Example 1. A contiguous matrix's strides fall directly out of its shape.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << "sizes=[" << m.size(0) << "," << m.size(1) << "]"
          << " strides=[" << m.stride(0) << "," << m.stride(1) << "]\n";
```

### Output

```text
sizes=[2,3] strides=[3,1]
```

Moving one row skips a whole row of `3` elements; moving one column skips
`1`. The last axis of a contiguous tensor always has stride `1`.

Example 2. Transposing swaps strides — the storage never moves.

```cpp
auto t = m.transpose(0, 1);
std::cout << "t sizes=[" << t.size(0) << "," << t.size(1) << "]"
          << " strides=[" << t.stride(0) << "," << t.stride(1) << "]"
          << " contiguous=" << std::boolalpha << t.is_contiguous() << "\n";
```

### Output

```text
t sizes=[3,2] strides=[1,3] contiguous=false
```

`t` shares `m`'s exact storage; only the metadata describing how to walk it
changed. Because the strides are no longer in decreasing order matching the
shape, `t` reports [`is_contiguous`](is_contiguous.md)` == false`.

Example 3. A broadcast axis has stride `0` — many logical coordinates share
one physical element.

```cpp
auto row = pluss::make_series({1.0, 2.0, 3.0});
auto wide = row.unsqueeze(0).expand({4, 3});
std::cout << "wide sizes=[" << wide.size(0) << "," << wide.size(1) << "]"
          << " strides=[" << wide.stride(0) << "," << wide.stride(1) << "]\n";
```

### Output

```text
wide sizes=[4,3] strides=[0,1]
```

Advancing along the broadcast axis (`dim = 0`, all four "rows") does not move
the address at all — every row reads the same three underlying values. This
is why writing through a broadcast view is dangerous: every stride-`0`
coordinate is the *same* storage cell.

Example 4. A stepped slice has a stride wider than the base it came from.

```cpp
auto v = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0});
auto every_other = v.slice(0, 0, 8, 2);
std::cout << "every_other numel=" << every_other.numel()
          << " stride(0)=" << every_other.stride(0) << "\n";
std::cout << every_other;
```

### Output

```text
every_other numel=4 stride(0)=2
pluss::Series(shape=[4], strides=[2], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[0, 2, 4, 6]
```

Every other element means skipping two storage slots per logical step, so
`stride(0) = 2` instead of the base's `1`. The view is `numel() == 4`, but
non-contiguous, because it visits only half of a contiguous run.

Example 5. A 3-D tensor's strides read outermost to innermost.

```cpp
auto t3 = pluss::series::zeros({2, 3, 4});
std::cout << "sizes=[2,3,4] strides=["
          << t3.stride(0) << "," << t3.stride(1) << "," << t3.stride(2) << "]\n";
```

### Output

```text
sizes=[2,3,4] strides=[12,4,1]
```

Advancing the outermost axis skips a whole `3 x 4 = 12`-element slab;
advancing the middle axis skips one `4`-element row; the innermost axis, as
always for a contiguous tensor, has stride `1`.

Example 6. An axis outside `[-dim(), dim() - 1]` raises.

```cpp
try {
  auto m2 = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  auto s = m2.stride(2);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << '\n';
}
```

### Output

```text
Series::stride: dim out of range
```

## Gotchas And Failure Modes

- Strides are measured in elements, not bytes. Multiply by
  [`dtype`](dtype.md)`().itemsize` for a byte offset.
- A stride of `0` means a broadcast axis — many coordinates share one
  physical element. Writing through such a view (where it is even permitted)
  affects every one of those coordinates at once.
- Non-contiguous is not invalid. Transposed, sliced-with-a-step, and
  broadcast views are all intentionally non-contiguous and remain fully
  usable by most operations.
- A scalar has no axes, so *every* call to `stride(dim)` is out of range; its
  `strides()` sequence is empty.
- Call [`contiguous`](contiguous.md) when a downstream operation genuinely
  requires a packed layout — not merely because a stride differs from the
  default contiguous one.

## Summary

`stride(dim)` / `strides()` report how far apart, in storage elements,
consecutive coordinates on an axis are:

- a fresh contiguous tensor's last axis always has stride `1`
- transposing and slicing-with-a-step change strides without moving data
- a broadcast axis has stride `0`
- negative `dim` normalizes against [`dim()`](dim.md); out-of-range axes
  raise

## See Also

- [`Series::is_contiguous`](is_contiguous.md): whether the current layout
  matches the default packed one.
- [`Series::size`](size.md) / [`Series::sizes`](sizes.md): the extent each
  stride steps across.
- [`Series::transpose`](transpose.md): the classic way to get non-default
  strides without copying.
- [`Series::contiguous`](contiguous.md): materialize a packed copy when one
  is required.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="storage_version.html">
    <span class="label">Previous</span>
    <span class="title">storage_version</span>
  </a>
  <a class="next-link" href="strides.html">
    <span class="label">Next</span>
    <span class="title">strides</span>
  </a>
</div>
