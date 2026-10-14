<!-- pluss-hand-authored -->

# pluss::series::randint

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::randint(std::int64_t lo,
                       std::int64_t hi,
                       sizes,
                       ScalarType dtype = ScalarType::Int64,
                       std::uint64_t seed = 0) -> Series

pluss::series::randint(std::int64_t lo,
                       std::int64_t hi,
                       sizes,
                       Config config,
                       std::uint64_t seed = 0) -> Series

pluss::series::randint(std::int64_t lo,
                       std::int64_t hi,
                       sizes,
                       std::uint64_t seed,
                       opts...) -> Series
```

## What It Builds

Builds integer-valued random samples on a half-open interval.

This is the discrete counterpart to [`rand_btw()`](rand_btw.md):

- `rand_btw(lo, hi, ...)` draws floating values in `[lo, hi)`
- `randint(lo, hi, ...)` draws integer values in `{lo, ..., hi - 1}`

So the most important rule to know up front is that `hi` is excluded.

## Parameters

**lo** (`std::int64_t`)

The smallest possible emitted integer.

**hi** (`std::int64_t`)

The exclusive upper bound.

**sizes**

The output shape.

**dtype** (`ScalarType`)

The destination integer dtype for the direct overload.

**config** (`Config`)

The config path for dtype/platform routing.

One current quirk is worth documenting explicitly:

- if `config.dtype` is left at the generic default `Float32`, the config path
  normalizes it back to `Int64`

That mirrors the "integer by policy" behavior already called out on
[`arange()`](arange.md).

## Returns

`Series`
    An integer series whose values lie in the half-open range `[lo, hi)`.

## Examples

Example 1. A seeded call produces a reproducible integer sample.

```cpp
std::cout << pluss::series::randint(3, 8, {6}, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[6, 3, 4, 4, 3, 7]
```

Every emitted value is at least `3` and strictly less than `8`. That is the
correct half-open integer contract.

Example 2. Very small ranges make the discrete half-open contract obvious.

```cpp
std::cout << pluss::series::randint(0, 2, {8}, pluss::ScalarType::Int32, 42);
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=int32, platform=cpu, numel=8, contiguous=true)
data=[1, 0, 0, 0, 0, 1, 0, 1]
```

Example 3. Matrix-shaped integer sampling follows the same bounds.

```cpp
std::cout << pluss::series::randint(10, 20, {2, 3}, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[17, 11, 12],
  [13, 10, 18]]
```

Example 4. Reusing the same explicit seed replays the same integer draws.

```cpp
auto a = pluss::series::randint(3, 8, {4}, pluss::ScalarType::Int64, 11);
auto b = pluss::series::randint(3, 8, {4}, pluss::ScalarType::Int64, 11);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[4, 4, 6, 5]
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[4, 4, 6, 5]
```

Example 5. The config path currently normalizes a generic `Float32` request
back to `Int64`.

```cpp
std::cout << pluss::series::randint(
    3,
    8,
    {4},
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[6, 3, 4, 4]
```

## Failure Modes

`hi` must be strictly greater than `lo`.

If not, the factory fails with:

```text
randint: expected hi > lo
```

Example 6. Invalid bounds fail immediately.

```cpp
try {
  std::cout << pluss::series::randint(5, 5, {3}, pluss::ScalarType::Int64, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
randint: expected hi > lo
```

## Notes

- The default dtype is `Int64`.
- The interval is half-open.
- The config path currently preserves integer output policy even if the config
  started at its generic `Float32` default.

## See Also

- [`pluss::series::rand_btw`](rand_btw.md): floating interval draws.
- [`pluss::series::randperm`](randperm.md): shuffled `0..n-1` without repeated
  values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand_btw.html">
    <span class="label">Previous</span>
    <span class="title">rand_btw</span>
  </a>
  <a class="next-link" href="randn.html">
    <span class="label">Next</span>
    <span class="title">randn</span>
  </a>
</div>
