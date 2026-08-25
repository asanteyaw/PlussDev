<!-- pluss-hand-authored -->

# pluss::series::rand_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::rand_like(const Series& like,
                         ScalarType dtype,
                         std::uint64_t seed = 0) -> Series

pluss::series::rand_like(const Series& like,
                         Config cfg = {},
                         std::uint64_t seed = 0) -> Series
```

## What It Builds

`rand_like()` keeps the geometry of an existing tensor and fills it with
uniform floating samples on `[0, 1)`.

This is the geometry-preserving sibling of [`rand()`](rand.md). The important
question it answers is not "what shape do I want?" but "I already have the
right shape; give me fresh random values that fit it."

## Verified Examples

Example 1. The direct overload keeps the source shape and uses the requested dtype.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::rand_like(like, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.74156487, 0.1599104, 0.27860114],
  [0.34419072, 0.03803017, 0.86822808]]
```

Example 2. Reusing the same explicit seed replays the same geometry-matched sample.

```cpp
auto a = pluss::series::rand_like(like, pluss::ScalarType::Float32, 5);
auto b = pluss::series::rand_like(like, pluss::ScalarType::Float32, 5);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0.38676804, 0.752307, 0.23270917],
  [0.099339411, 0.18796012, 0.38060892]]
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0.38676804, 0.752307, 0.23270917],
  [0.099339411, 0.18796012, 0.38060892]]
```

Example 3. Scalar references keep the same contract, so the result is scalar.

```cpp
auto like_scalar = pluss::series::make(5.0);
std::cout << pluss::series::rand_like(like_scalar, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.74156487
```

Example 4. A current quirk worth teaching directly: the config overload can
preserve the reference tensor's floating dtype instead of honoring a new one.

```cpp
std::cout << pluss::series::rand_like(
    like,
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.74156487, 0.1599104, 0.27860114],
  [0.34419072, 0.03803017, 0.86822808]]
```

## Notes

- `rand_like()` preserves geometry, not values.
- Seed semantics are the same as on [`rand()`](rand.md).
- The config overload currently follows "like dtype" behavior on this path.

## See Also

- [`pluss::series::rand`](rand.md): shape-first uniform random factory.
- [`pluss::series::randn_like`](randn_like.md): geometry-preserving normal samples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="full_like.html">
    <span class="label">Previous</span>
    <span class="title">full_like</span>
  </a>
  <a class="next-link" href="randn_like.html">
    <span class="label">Next</span>
    <span class="title">randn_like</span>
  </a>
</div>
