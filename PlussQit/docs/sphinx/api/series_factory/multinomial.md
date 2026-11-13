<!-- pluss-hand-authored -->

# pluss::series::multinomial

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::multinomial(std::span<const double> probs,
                                  std::int64_t num_samples,
                                  bool replacement = true,
                                  std::uint64_t seed = 0)

Series pluss::series::multinomial(std::span<const double> probs,
                                  std::int64_t num_samples,
                                  bool replacement,
                                  Config cfg,
                                  std::uint64_t seed = 0)
```

## What It Builds

`multinomial()` samples indices from a categorical distribution described by
`probs`.

That means the output values are not probabilities themselves. They are integer
positions into the probability vector:

- `0` means the first bucket was chosen
- `1` means the second bucket was chosen
- and so on

The overload that matters most semantically is `replacement`:

- `true`: the same bucket can be chosen again
- `false`: once chosen, a bucket is removed from future draws

## Verified Examples

Example 1. With replacement, high-probability buckets can repeat.

```cpp
const std::vector<double> probs = {0.1, 0.2, 0.7};
const std::span<const double> probs_span(probs.data(), probs.size());

std::cout << pluss::series::multinomial(probs_span, 6, true, 42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 2, 2, 1, 2, 0]
```

Example 2. Without replacement, indices stop repeating.

```cpp
std::cout << pluss::series::multinomial(probs_span, 3, false, 42);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[2, 1, 0]
```

That is the cleanest way to show the overload split: same probabilities,
different sampling contract.

Example 3. Explicit seeds replay categorical draws exactly.

```cpp
auto a = pluss::series::multinomial(probs_span, 6, true, 11);
auto b = pluss::series::multinomial(probs_span, 6, true, 11);

std::cout << a << "\n" << b;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[1, 2, 2, 2, 0, 1]
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[1, 2, 2, 2, 0, 1]
```

Example 4. A single sample is still returned as a rank-1 index vector.

```cpp
std::cout << pluss::series::multinomial(probs_span, 1, true, 42);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[2]
```

Example 5. Zero-probability buckets are never chosen.

```cpp
const std::vector<double> zero_bucket_probs = {0.0, 1.0, 0.0};
const std::span<const double> zero_bucket_span(zero_bucket_probs.data(), zero_bucket_probs.size());

std::cout << pluss::series::multinomial(zero_bucket_span, 4, true, 42);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 1, 1, 1]
```

Example 6. The config overload currently keeps the canonical `int64` output
dtype.

```cpp
std::cout << pluss::series::multinomial(
    probs_span,
    6,
    true,
    pluss::Config{}.with_dtype(pluss::ScalarType::Int32),
    42);
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[2, 2, 2, 1, 2, 0]
```

Example 7. Empty probability vectors fail immediately.

```cpp
try {
  const std::vector<double> empty_probs;
  const std::span<const double> empty_span(empty_probs.data(), empty_probs.size());
  std::cout << pluss::series::multinomial(empty_span, 3, true, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
multinomial: probs cannot be empty
```

Example 8. Sampling without replacement can fail even when `num_samples` looks
plausible, if there is not enough non-zero mass left.

```cpp
try {
  const std::vector<double> sparse_probs = {0.7, 0.0, 0.3};
  const std::span<const double> sparse_span(sparse_probs.data(), sparse_probs.size());
  std::cout << pluss::series::multinomial(sparse_span, 3, false, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
multinomial: not enough non-zero probability mass for sampling without replacement
```

Example 9. Negative sample counts are rejected.

```cpp
try {
  std::cout << pluss::series::multinomial(probs_span, -1, true, 42) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
multinomial: expected num_samples >= 0
```

## Notes

- Output values are indices, not probabilities.
- `replacement=false` changes the legality of some requests, not just the randomness pattern.
- The config overload currently keeps `int64` output.

## See Also

- [`pluss::series::poisson`](poisson.md): count sampling rather than categorical bucket sampling.
- [`pluss::series::randperm`](randperm.md): unique integer draws from a fixed canonical range instead of a weighted probability vector.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poisson.html">
    <span class="label">Previous</span>
    <span class="title">poisson</span>
  </a>
  <a class="next-link" href="complex.html">
    <span class="label">Next</span>
    <span class="title">complex</span>
  </a>
</div>
