<!-- pluss-hand-authored -->

# pluss::series::bernoulli

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
pluss::series::bernoulli(sizes,
                         double p = 0.5,
                         ScalarType dtype = ScalarType::Bool,
                         std::uint64_t seed = 0) -> Series

pluss::series::bernoulli(sizes,
                         double p,
                         Config cfg,
                         std::uint64_t seed = 0) -> Series
```

## What It Builds

`bernoulli()` emits two-valued random draws controlled by one probability `p`.

The conceptual result is always the same:

- success with probability `p`
- failure with probability `1 - p`

What changes with dtype is only the representation:

- `Bool` gives `true` / `false`
- numeric dtypes give `1` / `0`

## Verified Examples

Example 1. The default bool path makes the Bernoulli contract easiest to see.

```cpp
std::cout << pluss::series::bernoulli({8}, 0.5, pluss::ScalarType::Bool, 42);
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, true, false, true, false, true]
```

Example 2. Numeric dtypes keep the same randomness but encode outcomes as `0` and `1`.

```cpp
std::cout << pluss::series::bernoulli({8}, 0.25, pluss::ScalarType::Float32, 42);
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=float32, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 1, 0, 1, 0, 0]
```

Example 3. Scalar shapes are allowed.

```cpp
std::cout << pluss::series::bernoulli({}, 0.75, pluss::ScalarType::Bool, 42);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=false
```

Example 4. Higher-rank shapes follow the same probability contract.

```cpp
std::cout << pluss::series::bernoulli({2, 3}, 0.4, pluss::ScalarType::Bool, 42);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[false, false, false],
  [true, false, true]]
```

Example 5. `p = 0` collapses the distribution to all failures.

```cpp
std::cout << pluss::series::bernoulli({6}, 0.0, pluss::ScalarType::Bool, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[false, false, false, false, false, false]
```

Example 6. `p = 1` collapses the distribution to all successes.

```cpp
std::cout << pluss::series::bernoulli({6}, 1.0, pluss::ScalarType::Bool, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[true, true, true, true, true, true]
```

Example 7. The config overload changes representation, not Bernoulli meaning.

```cpp
std::cout << pluss::series::bernoulli(
    {8},
    0.25,
    pluss::Config{}.with_dtype(pluss::ScalarType::Int16),
    42);
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=int16, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 1, 0, 1, 0, 0]
```

Example 8. Explicit seeds replay Bernoulli draws exactly.

```cpp
auto a = pluss::series::bernoulli({8}, 0.5, pluss::ScalarType::Bool, 7);
auto b = pluss::series::bernoulli({8}, 0.5, pluss::ScalarType::Bool, 7);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, true, false, true, true, false, false]
pluss::Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, true, false, true, true, false, false]
```

Example 9. Invalid probabilities fail immediately.

```cpp
try {
  std::cout << pluss::series::bernoulli({4}, 1.5, pluss::ScalarType::Bool, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
bernoulli: expected 0 <= p <= 1
```

## Notes

- Bernoulli is logically boolean even when you request numeric output.
- `p` must lie in the closed interval `[0, 1]`.

## See Also

- [`pluss::series::poisson`](poisson.md): count-valued stochastic factory rather than binary outcomes.
- [`pluss::series::randint`](randint.md): discrete integer sampling without Bernoulli semantics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eye_like.html">
    <span class="label">Previous</span>
    <span class="title">eye_like</span>
  </a>
  <a class="next-link" href="poisson.html">
    <span class="label">Next</span>
    <span class="title">poisson</span>
  </a>
</div>
