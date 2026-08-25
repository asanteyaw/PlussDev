<!-- pluss-hand-authored -->

# pluss::series::randint_like

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::randint_like(const Series& like,
                            std::int64_t hi,
                            ScalarType dtype = ScalarType::Int64,
                            std::uint64_t seed = 0) -> Series

pluss::series::randint_like(const Series& like,
                            std::int64_t lo,
                            std::int64_t hi,
                            ScalarType dtype = ScalarType::Int64,
                            std::uint64_t seed = 0) -> Series

pluss::series::randint_like(const Series& like,
                            std::int64_t hi,
                            Config cfg,
                            std::uint64_t seed = 0) -> Series

pluss::series::randint_like(const Series& like,
                            std::int64_t lo,
                            std::int64_t hi,
                            Config cfg,
                            std::uint64_t seed = 0) -> Series
```

## What It Builds

`randint_like()` keeps the shape of `like` and fills it with integer draws.

The overload split matters here more than on `rand_like()`:

- `randint_like(like, hi, ...)` is shorthand for `lo = 0`
- `randint_like(like, lo, hi, ...)` gives full half-open interval control

So this page is really about two things at once:

- geometry reuse
- half-open integer bounds

## Verified Examples

Example 1. The `hi`-only form samples from `{0, ..., hi - 1}`.

```cpp
auto like = pluss::series::make({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << pluss::series::randint_like(like, 5, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[3, 0, 1],
  [1, 0, 4]]
```

Example 2. The full `lo` / `hi` overload uses the same shape but shifts the interval.

```cpp
std::cout << pluss::series::randint_like(like, 10, 15, pluss::ScalarType::Int32, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[13, 10, 11],
  [11, 10, 14]]
```

Example 3. The config path has a current quirk worth documenting explicitly:
it can preserve the source tensor's dtype policy rather than forcing `Int64`.

```cpp
std::cout << pluss::series::randint_like(
    like,
    10,
    15,
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[13, 10, 11],
  [11, 10, 14]]
```

That is one of the more surprising factory quirks in the current surface: the
config overload follows "like dtype" resolution here, not the plain integer
default seen on [`randint()`](randint.md).

Example 4. Scalar references take a different path here: the result becomes a
length-1 vector rather than a scalar tensor.

```cpp
auto like_scalar = pluss::series::make(5.0);
std::cout << pluss::series::randint_like(like_scalar, 5, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[3]
```

That is a real current quirk worth calling out because it differs from
`rand_like()` and `randn_like()`.

Example 5. Explicit seeds still replay integer draws exactly.

```cpp
auto a = pluss::series::randint_like(like, 10, 15, pluss::ScalarType::Int32, 7);
auto b = pluss::series::randint_like(like, 10, 15, pluss::ScalarType::Int32, 7);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[11, 10, 14],
  [12, 12, 11]]
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[11, 10, 14],
  [12, 12, 11]]
```

Example 6. Invalid bounds fail the same way they do on [`randint()`](randint.md).

```cpp
try {
  std::cout << pluss::series::randint_like(like, 5, 5, pluss::ScalarType::Int64, 42) << "\n";
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

- `hi` is exclusive, just as on [`randint()`](randint.md).
- The config overload currently has different dtype policy from the direct
  integer-dtype overload.

## See Also

- [`pluss::series::randint`](randint.md): shape-first integer sampling.
- [`pluss::series::eye_like`](eye_like.md): another geometry-preserving
  constructor, but deterministic and structured rather than random.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="randn_like.html">
    <span class="label">Previous</span>
    <span class="title">randn_like</span>
  </a>
  <a class="next-link" href="eye_like.html">
    <span class="label">Next</span>
    <span class="title">eye_like</span>
  </a>
</div>
