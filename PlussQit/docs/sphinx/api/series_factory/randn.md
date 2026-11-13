<!-- pluss-hand-authored -->

# pluss::series::randn

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::randn(sizes,
                            ScalarType dtype = Float32,
                            std::uint64_t seed = 0)

Series pluss::series::randn(sizes,
                            Config config,
                            std::uint64_t seed = 0)

Series pluss::series::randn(sizes,
                            std::uint64_t seed,
                            opts...)
```

## What It Builds

Builds floating samples from the standard normal distribution.

So the intended mental model is:

- mean `0`
- standard deviation `1`
- real random draws, not a guaranteed symmetric mini-batch

On a small sample you should not expect the printed mean to be exactly `0` or
the printed spread to be exactly `1`. Those are distribution properties, not
per-call promises.

## Parameters

**sizes**

The output shape.

**dtype** (`ScalarType`)

The destination floating dtype for the direct overload.

**config** (`Config`)

The config path for dtype/platform routing.

**seed** (`std::uint64_t`)

The explicit seed for reproducible draws.

The seed rules are the same as for [`rand()`](rand.md): a non-zero seed locks
the sequence, while `seed=0` uses the library's current RNG stream.

## Returns

`Series`
    A floating series of standard-normal samples.

## Examples

Example 1. A seeded draw produces reproducible 1D Gaussian samples.

```cpp
std::cout << pluss::series::randn({4}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.41471976, 0.65268123, -0.89188623, 1.3268336]
```

The sample contains both positive and negative values, as you would expect from
a mean-zero normal source, but nothing forces tiny batches to look visually
symmetric.

Example 2. The same constructor works for matrix shapes.

```cpp
std::cout << pluss::series::randn({2, 2}, pluss::ScalarType::Float32, 42);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[0.41471976, 0.65268123],
  [-0.89188623, 1.3268336]]
```

Example 3. An empty shape list gives a scalar standard-normal draw.

```cpp
std::cout << pluss::series::randn({}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.41471976
```

Example 4. Reusing the same seed replays the same standard-normal sample.

```cpp
auto a = pluss::series::randn({4}, pluss::ScalarType::Float32, 9);
auto b = pluss::series::randn({4}, pluss::ScalarType::Float32, 9);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.0038172735, -0.87428439, 0.35349265, -1.5901767]
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.0038172735, -0.87428439, 0.35349265, -1.5901767]
```

Example 5. The config path changes dtype policy, not the standard-normal
distribution.

```cpp
std::cout << pluss::series::randn(
    {4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.41471976, 0.65268123, -0.89188623, 1.3268336]
```

Example 6. Just like `rand()`, the zero-seed path can still be replayed if you
reset the library RNG state first.

```cpp
pluss::tils::rng_seed(515151u);
auto g0 = pluss::series::randn({4}, pluss::ScalarType::Float32);
pluss::tils::rng_seed(515151u);
auto g1 = pluss::series::randn({4}, pluss::ScalarType::Float32);

std::cout << g0 << "\n" << g1;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1.1236531, -0.96066457, -1.6021671, 1.3236331]
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[1.1236531, -0.96066457, -1.6021671, 1.3236331]
```

## Notes

- `randn()` is the base Gaussian constructor.
- Use [`normal()`](normal.md) when the natural parameters are mean and standard
  deviation rather than the standard-normal form.
- The config and option-stream overloads change routing and dtype policy, not
  the underlying distribution.

## See Also

- [`pluss::series::normal`](normal.md): shifted and scaled Gaussian draws.
- [`pluss::series::rand`](rand.md): uniform draws on `[0, 1)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="randint.html">
    <span class="label">Previous</span>
    <span class="title">randint</span>
  </a>
  <a class="next-link" href="normal.html">
    <span class="label">Next</span>
    <span class="title">normal</span>
  </a>
</div>
