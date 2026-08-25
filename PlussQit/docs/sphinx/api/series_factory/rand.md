<!-- pluss-hand-authored -->

# pluss::series::rand

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::rand(sizes,
                    ScalarType dtype = Float32,
                    std::uint64_t seed = 0) -> Series

pluss::series::rand(sizes,
                    Config config,
                    std::uint64_t seed = 0) -> Series

pluss::series::rand(sizes,
                    std::uint64_t seed,
                    opts...) -> Series
```

## What It Builds

Builds floating random samples from the uniform distribution on `[0, 1)`.

`rand()` is the base stochastic constructor for the whole factory family:

- [`rand_btw()`](rand_btw.md) rescales it to a caller-chosen interval
- [`randint()`](randint.md) turns it into discrete integer draws
- [`randperm()`](randperm.md) uses the same seeded-RNG idea for permutations

So this page is mostly about the seed and dtype rules that the rest of the
family inherits.

## Overload Map

**`rand(sizes, dtype, seed)`**

The direct shape + dtype + seed spelling.

**`rand(sizes, config, seed)`**

The config-based routing form.

**`rand(sizes, seed, opts...)`**

Option-stream sugar for callers that want the seed early and the config-like
options afterward.

## Parameters

**sizes**

The output shape.

**dtype** (`ScalarType`)

The destination floating dtype for the simple overload.

**config** (`Config`)

The config path for dtype/platform routing.

**seed** (`std::uint64_t`)

The RNG seed for reproducible draws.

## Returns

`Series`
    A floating series of the requested shape containing uniform samples in
    `[0, 1)`.

## Seed Semantics

Two rules matter in practice:

- a non-zero explicit seed gives reproducible output for the same shape, dtype,
  and backend
- `seed=0` means "use the library's current RNG stream," not "force
  nondeterminism"

That second rule is easy to miss. The zero-seed path is still deterministic if
the process-level RNG state has been reset through `pluss::rng_seed(...)`.

## Examples

Example 1. An explicit seed gives a reproducible 1D uniform sample.

```cpp
std::cout << pluss::series::rand({4}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.74156487, 0.1599104, 0.27860114, 0.34419072]
```

Every value is greater than or equal to `0` and strictly less than `1`.

Example 2. The same constructor works for higher-rank shapes.

```cpp
std::cout << pluss::series::rand({2, 3}, pluss::ScalarType::Float32, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float32, platform=cpu, numel=6, contiguous=true)
data=[[0.74156487, 0.1599104, 0.27860114],
  [0.34419072, 0.03803017, 0.86822808]]
```

Example 3. An empty shape list gives a scalar random draw.

```cpp
std::cout << pluss::series::rand({}, pluss::ScalarType::Float64, 42);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.74156487
```

Example 4. Reusing the same explicit seed replays the same sequence exactly.

```cpp
auto a = pluss::series::rand({4}, pluss::ScalarType::Float32, 7);
auto b = pluss::series::rand({4}, pluss::ScalarType::Float32, 7);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.38982975, 0.016788295, 0.90076071, 0.58293027]
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.38982975, 0.016788295, 0.90076071, 0.58293027]
```

Example 5. The zero-seed path is still replayable when you reset the library
RNG state.

```cpp
pluss::rng_seed(424242u);
auto g0 = pluss::series::rand({4}, pluss::ScalarType::Float32);
pluss::rng_seed(424242u);
auto g1 = pluss::series::rand({4}, pluss::ScalarType::Float32);

std::cout << g0 << "\n" << g1;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.48587129, 0.69873297, 0.74466199, 0.19883089]
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.48587129, 0.69873297, 0.74466199, 0.19883089]
```

Example 6. The config overload changes dtype and routing policy, not the
uniform distribution itself.

```cpp
std::cout << pluss::series::rand(
    {4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[0.74156487, 0.1599104, 0.27860114, 0.34419072]
```

## Notes

- `rand()` is a floating factory; it is the wrong constructor when you need
  integer draws or permutations.
- `seed=0` uses the current library RNG stream and can therefore still be
  replayed after `pluss::rng_seed(...)`.
- The config path changes routing and dtype policy, not the fundamental
  distribution.

## See Also

- [`pluss::series::rand_btw`](rand_btw.md): uniform samples on a caller-chosen
  interval.
- [`pluss::series::randn`](randn.md): standard-normal samples instead of
  uniform samples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="zeros.html">
    <span class="label">Previous</span>
    <span class="title">zeros</span>
  </a>
  <a class="next-link" href="rand_btw.html">
    <span class="label">Next</span>
    <span class="title">rand_btw</span>
  </a>
</div>
