<!-- pluss-hand-authored -->

# pluss::series::empty_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::empty_like(const Series& like,
                                 ScalarType dtype)

Series pluss::series::empty_like(const Series& like,
                                 Config cfg = {})

Series pluss::series::empty_like(const Series& like,
                                 opts...)
```

## What It Builds

`empty_like()` is the "match this tensor's geometry" version of
[`empty()`](empty.md).

That means:

- same shape as `like`
- same platform as `like`
- same dtype by default unless you override it

The important warning is the same as for ordinary `empty()`: the payload is not
document-safe until you write into it.

## Verified Examples

Example 1. Treat `empty_like()` as allocation only, then initialize it before
printing or using it.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

auto out = pluss::series::empty_like(like);
out.copy_(pluss::series::make({
    {9.0, 8.0, 7.0},
    {6.0, 5.0, 4.0},
}));

std::cout << out;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[9, 8, 7],
  [6, 5, 4]]
```

Example 2. The dtype override changes storage policy while keeping the shape.

```cpp
auto out_i16 = pluss::series::empty_like(like, pluss::ScalarType::Int16);
out_i16.copy_(pluss::series::make({
    {9, 8, 7},
    {6, 5, 4},
}, pluss::ScalarType::Int16));

std::cout << out_i16;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int16, platform=cpu, numel=6, contiguous=true)
data=[[9, 8, 7],
  [6, 5, 4]]
```

Example 3. Scalar references stay scalar.

```cpp
auto like_scalar = pluss::series::make(5.0);
auto out = pluss::series::empty_like(like_scalar);
out.copy_(pluss::series::make(-9.5));

std::cout << out;
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-9.5
```

Example 4. The config overload changes metadata policy, not the write-first
discipline.

```cpp
auto out_bool = pluss::series::empty_like(
    like,
    pluss::Config{}.with_dtype(pluss::ScalarType::Bool));
out_bool.copy_(pluss::series::make({
    {true, false, true},
    {false, true, false},
}, pluss::ScalarType::Bool));

std::cout << out_bool;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, true],
  [false, true, false]]
```

## Notes

- Never rely on the raw payload of `empty_like()` before you fill it.
- The point of the factory is geometry reuse, not payload semantics.

## See Also

- [`pluss::series::empty`](empty.md): ordinary shape-first uninitialized allocation.
- [`pluss::series::zeros_like`](zeros_like.md): same geometry, but safely initialized.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="randperm.html">
    <span class="label">Previous</span>
    <span class="title">randperm</span>
  </a>
  <a class="next-link" href="zeros_like.html">
    <span class="label">Next</span>
    <span class="title">zeros_like</span>
  </a>
</div>
