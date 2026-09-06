<!-- pluss-hand-authored -->

# Series::numel

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::int64_t Series::numel() const
```

## What It Does

Returns the total number of logical values in the `Series` — the product of
every shape extent.

This is the complementary question to [`dim`](dim.md): `dim()` asks how many
axes there are, `numel()` asks how many values live inside them. A scalar
holds exactly one value, so `numel()` is `1` regardless of its rank-`0`
shape. If *any* axis has extent `0`, the whole product collapses to `0` — a
`[5, 0, 7]` tensor has three axes and zero values.

`numel()` is a **logical** count, deliberately blind to physical layout.
Transposing, permuting, or otherwise changing a view's strides never changes
how many values it addresses — only where those addresses land in storage.

The call is a plain native-integer accessor: no allocation, no data
movement.

## Parameters

None.

## Returns

`std::int64_t`
    The product of every entry in [`sizes()`](sizes.md). Always `>= 0`; `0`
    exactly when some axis has extent `0`.

## Gradient

Not applicable — `numel()` returns a plain C++ integer, not a `Series`.

## Examples

Example 1. The product of the shape extents.

```cpp
auto s = pluss::make_series(7.0);
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << "scalar numel=" << s.numel() << "\n";
std::cout << "matrix numel=" << m.numel() << "\n";
```

### Output

```text
scalar numel=1
matrix numel=6
```

`2 * 3 = 6` for the matrix; the scalar's empty shape multiplies to the
identity, `1`.

Example 2. One empty axis zeroes the entire product, no matter how large the
others are.

```cpp
auto e = pluss::series::zeros({5, 0, 7});
std::cout << "shape=[5,0,7] numel=" << e.numel() << "\n";
```

### Output

```text
shape=[5,0,7] numel=0
```

Example 3. Transposing rearranges strides, never `numel()` — the same
values, addressed in a different order.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto t = m.transpose(0, 1);
std::cout << "m numel=" << m.numel() << ", t numel=" << t.numel() << "\n";
```

### Output

```text
m numel=6, t numel=6
```

Example 4. `numel() == 0` is the correct emptiness test — `dim() == 0` tests
something else entirely (a scalar).

```cpp
auto scalar = pluss::make_series(7.0);
auto empty = pluss::series::zeros({0});
std::cout << "scalar: dim=" << scalar.dim() << " numel=" << scalar.numel() << " -- not empty\n";
std::cout << "empty:  dim=" << empty.dim() << " numel=" << empty.numel() << " -- empty\n";
```

### Output

```text
scalar: dim=0 numel=1 -- not empty
empty:  dim=1 numel=0 -- empty
```

A scalar has rank `0` and *is* one value; a rank-`1` empty vector has an axis
but nothing on it. Only `numel()` tells them apart correctly.

Example 5. Total storage in bytes is `numel()` times [`dtype`](dtype.md)`().itemsize`
— `numel()` alone is a value count, not a byte count.

```cpp
auto x = pluss::series::zeros({100}, pluss::ScalarType::Float64);
std::cout << "numel=" << x.numel() << ", itemsize=" << x.dtype().itemsize
          << ", bytes=" << x.numel() * static_cast<std::int64_t>(x.dtype().itemsize) << "\n";
```

### Output

```text
numel=100, itemsize=8, bytes=800
```

## Gotchas And Failure Modes

- `numel()` is not a byte count. Multiply by
  [`dtype`](dtype.md)`().itemsize` when the payload size in bytes is what you
  actually need.
- `numel() == 1` does not mean "scalar" — a `[1]` vector and a `[1, 1]`
  matrix both have `numel() == 1` but nonzero rank. Test rank with
  [`dim`](dim.md) if the shape itself matters, not just the count.
- Views never change `numel()`. A non-contiguous transpose, an expanded
  broadcast, and a stepped slice can all report a `numel()` equal to (or, for
  a broadcast, larger than) their base without physically holding that many
  distinct storage cells.
- An axis of extent `0` anywhere collapses the whole product — it is not
  simply ignored.

## Summary

`numel()` is the total logical value count — the product of every shape
extent:

- independent of [`dim`](dim.md); a wide rank-1 tensor can hold more values
  than a deep high-rank one
- `0` whenever any axis has extent `0`
- unaffected by transposing, permuting, or any other stride-only view change
- not a byte count — multiply by `dtype().itemsize` for that

## See Also

- [`Series::dim`](dim.md): the axis count — the complementary structural
  question to value count.
- [`Series::size`](size.md) / [`Series::sizes`](sizes.md): the extent
  contributing to the product, per axis or all at once.
- [`Series::dtype`](dtype.md): bytes per value, for converting a value count
  into a byte count.
- [`Series::is_nonzero`](is_nonzero.md): the boolean truth value of exactly
  one element — requires `numel() == 1`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normal_.html">
    <span class="label">Previous</span>
    <span class="title">normal_</span>
  </a>
  <a class="next-link" href="outer.html">
    <span class="label">Next</span>
    <span class="title">outer</span>
  </a>
</div>
