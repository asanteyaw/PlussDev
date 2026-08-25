<!-- pluss-hand-authored -->

# pluss::series::ones_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::ones_like(const Series& like,
                         ScalarType dtype) -> Series

pluss::series::ones_like(const Series& like,
                         Config cfg = {}) -> Series

pluss::series::ones_like(const Series& like,
                         opts...) -> Series
```

## What It Builds

`ones_like()` matches an existing tensor's geometry and fills the result with
`1`.

That makes it the geometry-preserving counterpart to [`ones()`](ones.md), with
the same "constant one" semantics but a different starting point:

- `ones(...)`: start from a requested shape
- `ones_like(like, ...)`: start from an existing tensor's shape

## Verified Examples

Example 1. By default the result keeps the source tensor's dtype.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::ones_like(like);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 1, 1],
  [1, 1, 1]]
```

Example 2. Overriding the dtype changes how that constant one is stored.

```cpp
std::cout << pluss::series::ones_like(like, pluss::ScalarType::Bool);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, true],
  [true, true, true]]
```

For boolean outputs, the "one" interpretation naturally becomes `true`.

Example 3. Scalar references produce scalar one outputs.

```cpp
auto like_scalar = pluss::series::make(5.0);
std::cout << pluss::series::ones_like(like_scalar);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
```

Example 4. The config overload can redirect the destination dtype while
keeping the same geometry.

```cpp
std::cout << pluss::series::ones_like(
    like,
    pluss::Config{}.with_dtype(pluss::ScalarType::Int16));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int16, platform=cpu, numel=6, contiguous=true)
data=[[1, 1, 1],
  [1, 1, 1]]
```

## Notes

- Default dtype follows `like`.
- Boolean ones become `true`, not integer `1`.

## See Also

- [`pluss::series::zeros_like`](zeros_like.md): same geometry, zero-filled.
- [`pluss::series::full_like`](full_like.md): same geometry, caller-chosen fill.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="zeros_like.html">
    <span class="label">Previous</span>
    <span class="title">zeros_like</span>
  </a>
  <a class="next-link" href="full_like.html">
    <span class="label">Next</span>
    <span class="title">full_like</span>
  </a>
</div>
