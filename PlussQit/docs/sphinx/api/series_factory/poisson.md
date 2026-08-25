<!-- pluss-hand-authored -->

# pluss::series::poisson

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::poisson(sizes,
                       double lambda,
                       ScalarType dtype = ScalarType::Int64,
                       std::uint64_t seed = 0) -> Series

pluss::series::poisson(sizes,
                       double lambda,
                       Config cfg,
                       std::uint64_t seed = 0) -> Series
```

## What It Builds

`poisson()` emits non-negative count samples controlled by one rate parameter,
`lambda`.

This is the stochastic count factory in the public constructor surface, so the
first teaching rule is simple but important:

- outputs are counts
- counts cannot be negative
- `lambda = 0` collapses the distribution to all zeros

## Verified Examples

Example 1. A seeded Poisson draw produces integer counts.

```cpp
std::cout << pluss::series::poisson({8}, 3.0, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[3, 2, 2, 0, 7, 1, 2, 1]
```

Example 2. `lambda = 0` degenerates cleanly to zero counts.

```cpp
std::cout << pluss::series::poisson({6}, 0.0, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[0, 0, 0, 0, 0, 0]
```

Example 3. Scalar Poisson draws are allowed.

```cpp
std::cout << pluss::series::poisson({}, 3.0, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=3
```

Example 4. Matrix-shaped count sampling uses the same non-negative count
contract.

```cpp
std::cout << pluss::series::poisson({2, 3}, 2.0, pluss::ScalarType::Int32, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[[3, 1, 2],
  [1, 5, 1]]
```

Example 5. Small positive rates usually emit many zeros, but not necessarily
all zeros.

```cpp
std::cout << pluss::series::poisson({8}, 0.25, pluss::ScalarType::Int64, 42);
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 0, 0, 0]
```

Example 6. The config path currently normalizes a generic floating default back
to integer-count storage.

```cpp
std::cout << pluss::series::poisson(
    {6},
    2.0,
    pluss::Config{}.with_dtype(pluss::ScalarType::Float32),
    42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[3, 1, 2, 1, 5, 1]
```

Example 7. Explicit seeds replay count draws exactly.

```cpp
auto a = pluss::series::poisson({6}, 2.0, pluss::ScalarType::Int64, 7);
auto b = pluss::series::poisson({6}, 2.0, pluss::ScalarType::Int64, 7);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 1, 0, 4, 2, 5]
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 1, 0, 4, 2, 5]
```

Example 8. Negative rates are rejected.

```cpp
try {
  std::cout << pluss::series::poisson({4}, -1.0, pluss::ScalarType::Int64, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
poisson: expected finite lambda >= 0
```

## Notes

- `lambda` must be finite and non-negative.
- The config path keeps count semantics even if the incoming config started as a floating default.

## See Also

- [`pluss::series::bernoulli`](bernoulli.md): binary outcomes rather than counts.
- [`pluss::series::multinomial`](multinomial.md): categorical index sampling rather than scalar count sampling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bernoulli.html">
    <span class="label">Previous</span>
    <span class="title">bernoulli</span>
  </a>
  <a class="next-link" href="multinomial.html">
    <span class="label">Next</span>
    <span class="title">multinomial</span>
  </a>
</div>
