<!-- pluss-hand-authored -->

# Series::log_normal_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::log_normal_() const
Series& Series::log_normal_(double mean) const
Series& Series::log_normal_(double mean, double stddev) const
```

## What It Does

Overwrites every element of the receiver with `exp(z)` where `z` is a normal
draw:

```text
z ~ Normal(mean, stddev)
x = exp(z)
```

So every sample is **strictly positive** and the distribution is
right-skewed. The catch is that `mean` and `stddev` describe the *log-scale*
variable `z`, **not** the output `x`. For the output:

- the **median** is `exp(mean)`
- the **arithmetic mean** is `exp(mean + stddev*stddev/2)` — always larger
  than the median, and it grows fast with `stddev` because exponentiation
  magnifies the upper tail.

To target a desired output **median** `m`, pass `mean = log(m)`.

Defaults are `mean = 1`, `stddev = 2` (of the underlying normal). The
one-argument overload sets `mean` and keeps `stddev = 1`.

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the values; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**mean** (`double`, default `1`): the mean of the underlying normal `z`. Must
be finite.

**stddev** (`double`, default `2`): the standard deviation of `z`. Must be
finite and `> 0`.

## Returns

`Series&`
    A reference to the receiver, now holding the samples. The write is
    in-place and preserves storage identity — every alias sees the values.

## Raises

- if the receiver's dtype is not floating-point
- if `mean` is non-finite, or `stddev` is non-finite or `<= 0`

## Gradient

Not differentiable.

## Examples

Example 1. Default parameters (`z ~ Normal(1, 2)`). Every value is positive;
the tail is heavy. Seeded, so fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({6}, pluss::ScalarType::Float64);
x.log_normal_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[6.5764, 5.8162, 5.1609, 70.2873, 0.3596, 0.9298]
```

`70.29` in a sample of six is the right tail — with `stddev = 2` on the log
scale, that is expected.

Example 2. The parameters describe `z`, not `x`. With `z ~ Normal(0, 0.5)`,
the output median lands on `exp(0) = 1` and the output mean on
`exp(0.125) ≈ 1.133`.

```cpp
pluss::rng_seed(7);
auto x = pluss::series::empty({500000}, pluss::ScalarType::Float64);
x.log_normal_(0.0, 0.5);
std::cout << "output median~" << x.median().to_vec<double>()[0]
          << "  output mean~" << x.mean().to_vec<double>()[0] << "\n";
```

### Output

```text
output median~1.00057  output mean~1.13324
```

The mean sits above the median — the signature of a right-skewed
distribution.

Example 3. Non-floating receiver, `stddev <= 0`, and a non-finite `mean` are
all rejected.

```cpp
try {
  auto i = pluss::series::empty({3}, pluss::ScalarType::Int64);
  i.log_normal_();
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.log_normal_(0.0, 0.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.log_normal_(std::numeric_limits<double>::quiet_NaN(), 1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
log_normal_: receiver must have a floating-point dtype
log_normal_: expected stddev > 0
log_normal_: expected a finite mean
```

Example 4. Reproducible under a fixed seed, and visible through aliases.

```cpp
pluss::rng_seed(5);
auto a = pluss::series::empty({6}, pluss::ScalarType::Float64); a.log_normal_(0.0, 1.0);
pluss::rng_seed(5);
auto b = pluss::series::empty({6}, pluss::ScalarType::Float64);
auto alias = b;
b.log_normal_(0.0, 1.0);
std::cout << std::boolalpha
          << "a == b: " << a.equal(b)
          << "  alias.is_set_to(b): " << alias.is_set_to(b) << "\n";
```

### Output

```text
a == b: true  alias.is_set_to(b): true
```

## Gotchas And Failure Modes

- `mean` and `stddev` are **log-scale** parameters. The output median is
  `exp(mean)`; the output mean is `exp(mean + stddev*stddev/2)`. Do not read
  `mean` as "the average value I want".
- The arithmetic mean grows steeply with `stddev`. A moderate `stddev` on the
  log scale produces a very heavy output tail.
- Floating-point receiver only; `stddev` must be strictly positive.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`log_normal_(mean = 1, stddev = 2)` fills the receiver in place with
`exp(Normal(mean, stddev))` samples:

- strictly positive, right-skewed
- parameters describe the underlying normal: output median `exp(mean)`,
  output mean `exp(mean + stddev^2/2)`
- floating-point receiver only; `stddev > 0`
- reproducible under `pluss::rng_seed(seed)`; not differentiable; aliases
  see the write

## See Also

- [`Series::normal_`](normal_.md): the underlying, untransformed variable.
- [`Series::exponential_`](exponential_.md): a different positive,
  right-skewed law (with finite moments and a lighter tail).
- [`Series::cauchy_`](cauchy_.md): symmetric but heavier-tailed — and with
  no finite mean at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="linearalg.html">
    <span class="label">Previous</span>
    <span class="title">linearalg</span>
  </a>
  <a class="next-link" href="log.html">
    <span class="label">Next</span>
    <span class="title">log</span>
  </a>
</div>
