<!-- pluss-hand-authored -->

# pluss::series::randn_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::randn_like(const Series& like,
                                 ScalarType dtype,
                                 std::uint64_t seed = 0)

Series pluss::series::randn_like(const Series& like,
                                 Config cfg = {},
                                 std::uint64_t seed = 0)
```

## What It Builds

`randn_like()` preserves an existing tensor's geometry and fills it with
standard-normal samples.

It is the like-factory counterpart to [`randn()`](randn.md): same geometry
reuse idea as `rand_like()`, but with the normal distribution instead of the
uniform one.

## Verified Examples

Example 1. A seeded call preserves shape while drawing from the standard normal.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::randn_like(like, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.41471976, 0.65268123, -0.89188623],
  [1.3268336, 1.729593, -1.8834168]]
```

Example 2. Reusing the same explicit seed replays the same geometry-matched
normal sample.

```cpp
auto a = pluss::series::randn_like(like, pluss::ScalarType::Float32, 5);
auto b = pluss::series::randn_like(like, pluss::ScalarType::Float32, 5);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0.019979106, -1.3782094, 1.3856395],
  [0.99796528, -1.3376261, 1.2465181]]
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0.019979106, -1.3782094, 1.3856395],
  [0.99796528, -1.3376261, 1.2465181]]
```

Example 3. Scalar references stay scalar.

```cpp
auto like_scalar = pluss::series::make(5.0);
std::cout << pluss::series::randn_like(like_scalar, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.41471976
```

Example 4. The config overload currently preserves the reference tensor's
floating dtype on this path too.

```cpp
std::cout << pluss::series::randn_like(
    like,
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.41471976, 0.65268123, -0.89188623],
  [1.3268336, 1.729593, -1.8834168]]
```

## Notes

- Geometry follows `like`.
- Distribution and seed policy follow [`randn()`](randn.md).
- The config overload currently follows "like dtype" behavior rather than
  force-switching to the requested floating dtype.

## See Also

- [`pluss::series::rand_like`](rand_like.md): geometry-preserving uniform samples.
- [`pluss::series::randn`](randn.md): shape-first standard-normal factory.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand_like.html">
    <span class="label">Previous</span>
    <span class="title">rand_like</span>
  </a>
  <a class="next-link" href="randint_like.html">
    <span class="label">Next</span>
    <span class="title">randint_like</span>
  </a>
</div>
