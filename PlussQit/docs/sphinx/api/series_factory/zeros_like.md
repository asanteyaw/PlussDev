<!-- pluss-hand-authored -->

# pluss::series::zeros_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::zeros_like(const Series& like,
                          ScalarType dtype) -> Series

pluss::series::zeros_like(const Series& like,
                          Config cfg = {}) -> Series

pluss::series::zeros_like(const Series& like,
                          opts...) -> Series
```

## What It Builds

`zeros_like()` keeps the geometry of an existing tensor and safely fills the
entire result with zero.

This is often the more practical sibling of [`empty_like()`](empty_like.md)
because you get:

- same shape as `like`
- predictable initialization
- optional dtype override when you want a different storage policy

The public surface also keeps the British-spelling alias `zeroes_like()`, but
`zeros_like()` is the canonical spelling used throughout the docs.

## Verified Examples

Example 1. The default path preserves the source tensor's dtype.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::zeros_like(like);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0, 0, 0],
  [0, 0, 0]]
```

Example 2. An explicit dtype override keeps the shape but changes storage.

```cpp
std::cout << pluss::series::zeros_like(like, pluss::ScalarType::Int16);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int16, platform=cpu, numel=6, contiguous=true)
data=[[0, 0, 0],
  [0, 0, 0]]
```

Example 3. Scalar references produce scalar zero outputs.

```cpp
auto like_scalar = pluss::series::make(5.0);
std::cout << pluss::series::zeros_like(like_scalar);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 4. The config overload can change the stored zero representation while
keeping the same geometry.

```cpp
std::cout << pluss::series::zeros_like(
    like,
    pluss::Config{}.with_dtype(pluss::ScalarType::Bool));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[false, false, false],
  [false, false, false]]
```

## Notes

- `zeros_like()` is the safe geometry-preserving allocation surface.
- `zeroes_like()` is a compatibility alias, not a different algorithm.

## See Also

- [`pluss::series::empty_like`](empty_like.md): same geometry without initialization.
- [`pluss::series::ones_like`](ones_like.md): same geometry, but unit-filled.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty_like.html">
    <span class="label">Previous</span>
    <span class="title">empty_like</span>
  </a>
  <a class="next-link" href="ones_like.html">
    <span class="label">Next</span>
    <span class="title">ones_like</span>
  </a>
</div>
