<!-- pluss-hand-authored -->

# Series::flatten

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::flatten() const
Series Series::flatten(std::int64_t start_dim) const
Series Series::flatten(std::int64_t start_dim, std::int64_t end_dim) const
```

## What It Does

Collapses a contiguous range of axes into one axis.

`flatten()` is a shape-composition helper, not a data-dependent transform. It
computes a destination shape and then delegates to [`reshape()`](reshape.md),
which means it inherits reshape’s layout policy:

- if the current layout can be reinterpreted directly, the result is a view
- if not, a contiguous materialization happens first

That is why `flatten()` works on both contiguous and non-contiguous inputs
while still preserving the logical value order of the input it was given.

## Parameters

**start_dim** (`std::int64_t`)

The first axis to include in the flattened range. Defaults to `0`.

**end_dim** (`std::int64_t`)

The last axis to include in the flattened range. Defaults to `-1`, meaning the
last axis.

Both parameters accept negative indexing. After normalization:

- both must be valid axis indices
- `start_dim` must be less than or equal to `end_dim`

For scalars, `flatten()` now produces a length-`1` vector.

## Returns

`Series`
    A `Series` whose selected axis range has been replaced by one axis whose
    length is the product of the removed axes' lengths.

## Gradient

`flatten()` is differentiable. Backward reshapes the outgoing sensitivity back
to the original shape.

This is exactly what you want from a pure shape op: the sensitivity keeps the
same element ordering, just expressed again under the original rank.

## Examples

The examples on this page build incrementally.

Example 1. The zero-argument overload flattens the whole `Series`.

```cpp
auto cube = pluss::make_series({
    {
        {1.0, 2.0},
        {3.0, 4.0},
    },
    {
        {5.0, 6.0},
        {7.0, 8.0},
    },
});
std::cout << cube.flatten();
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1, 2, 3, 4, 5, 6, 7, 8]
```

All three axes collapse into one length-`8` axis.

Example 2. The one-argument overload keeps the leading axes and flattens from
`start_dim` to the end.

```cpp
std::cout << cube.flatten(1);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 3, 4],
  [5, 6, 7, 8]]
```

Axis `0` stays intact. Axes `1` and `2` are multiplied together into one axis
of length `4`.

Example 3. The two-argument overload lets you flatten only a selected axis
window.

```cpp
std::cout << cube.flatten(0, 1);
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2],
  [3, 4],
  [5, 6],
  [7, 8]]
```

Here axes `0` and `1` collapse into one axis of length `4`, while the final
axis is preserved.

Example 4. On a non-contiguous input, `flatten()` preserves logical order, not
the parent’s original physical layout.

```cpp
std::cout << cube.transpose(0, 1).flatten(1, 2);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 5, 6],
  [3, 4, 7, 8]]
```

That output reflects the transposed logical view first, then the flatten. This
is the practical consequence of `flatten()` delegating through
[`reshape()`](reshape.md): if a copy is needed, the copy is made from the
current logical ordering of the input, not from some earlier parent layout.

Example 5. Flattening a scalar produces a length-`1` vector.

```cpp
auto scalar = pluss::make_series(42.0);
std::cout << scalar.flatten();
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[42]
```

Example 6. Invalid dimension windows are rejected.

```cpp
try {
  std::cout << cube.flatten(2, 1);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
flatten: start_dim must be <= end_dim
```

## Notes

- `flatten()` is a reshape-style op, not an indexing op.
- The default overload is equivalent to flattening from axis `0` through the
  last axis.
- Non-contiguous inputs are supported through the same fallback policy as
  [`reshape()`](reshape.md).

## See Also

- [`Series::reshape`](reshape.md): the more general shape-change primitive that
  `flatten()` builds on.
- [`Series::squeeze`](squeeze.md): remove size-`1` axes instead of multiplying a
  whole axis range together.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill_diagonal.html">
    <span class="label">Previous</span>
    <span class="title">fill_diagonal</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
