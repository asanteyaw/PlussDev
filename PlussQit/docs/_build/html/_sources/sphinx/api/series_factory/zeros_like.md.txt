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

`zeros_like()` allocates a new `Series` whose construction policy is derived
from an existing `Series`, then writes zero to every logical element. It is a
structural factory, not a conversion and not a view: the result owns separate
storage, so changing it cannot change `like`.

This is often the more practical sibling of [`empty_like()`](empty_like.md)
because initialization is part of the contract rather than a responsibility
left to the caller.

With no override, the factory carries forward every supported property needed
to make the result usable beside `like`:

- same shape as `like`
- same dtype
- same platform, including MPS or CUDA
- same dense stride order; a transposed source therefore produces a
  transposed-layout result rather than silently becoming row-major

For a sliced or overlapping source, exact strides may contain gaps or aliases
that do not belong in a fresh allocation. In that case `zeros_like()` compacts
the result into dense, non-overlapping storage while retaining the source's
dimension order. This follows the same `Preserve` policy used by the local
authoritative tensor implementation.

| Property | Default | Explicit override |
| --- | --- | --- |
| Shape | inherited from `like` | not supported; use `zeros()` for a new shape |
| Dtype | inherited from `like` | `ScalarType`, `Config::with_dtype()`, or `cfg::dtype()` |
| Platform | inherited from `like` | `Config::with_platform()` or `cfg::platform()` |
| Stride order | preserved and densified when necessary | no public memory-format override |
| Gradient tracking | disabled on the new leaf | not a factory option |
| Values | all zero in the chosen dtype | not configurable; use `full_like()` for another value |

The override is explicit even when it names an ordinary factory default. For
example, `with_dtype(Float32)` must convert an `Int16` reference to `Float32`,
and `with_platform(pCPU)` must move a result derived from an MPS reference to
CPU. Neither `Float32` nor CPU is treated as an “option was omitted” sentinel.

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

- `zeros_like()` never shares storage with `like`.
- Dtype and platform overrides are independent: changing one still inherits
  the other.
- A tracked source does not make the new `Series` tracked. This matches the
  fresh-leaf behavior of the authoritative implementation rather than treating
  allocation as a differentiable operation.
- Pinning is not inherited. The source implementation's tensor options do not
  carry source pinning, and Pluss does not currently expose pinned storage as a
  working `Series` allocation mode.

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
