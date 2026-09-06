<!-- pluss-hand-authored -->

# Series::normal_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::normal_() const
Series& Series::normal_(double mean) const
Series& Series::normal_(double mean, double stddev) const
```

## What It Does

Overwrites every element of the receiver with an independent draw from a
normal (Gaussian) distribution.

The receiver supplies the **shape, dtype, platform, and storage**; its
current values play no part and are simply replaced. This is the in-place
"fill this buffer with noise" primitive — the counterpart of the factory
[`randn`](../series_factory/randn.md), which *creates* a new tensor from a
shape you pass it. Reach for `normal_` when a buffer already exists (a
pre-allocated parameter, a reused scratch tensor, a slice of a larger array).

The three overloads set the two parameters:

| call | mean | stddev |
|---|---|---|
| `normal_()` | `0` | `1` |
| `normal_(m)` | `m` | `1` |
| `normal_(m, s)` | `m` | `s` |

`stddev` is the **standard deviation**, not the variance — `normal_(0, 2)`
has variance `4`. `stddev = 0` is allowed and produces a deterministic fill
with `mean` (a useful stand-in when a pipeline that normally samples needs to
be made reproducible for a test).

### Reproducibility

The draw is deterministic given the process-wide RNG seed. Call
`pluss::rng_seed(seed)` before sampling and the exact sequence of values is
fixed; two runs from the same seed produce `equal` tensors. Each sampling
call advances a stream counter, so consecutive draws differ unless you reseed
between them.

## Parameters

**mean** (`double`, default `0`): the center of the distribution. Must be
finite.

**stddev** (`double`, default `1`): the spread. Must be finite and `>= 0`.

## Returns

`Series&`
    A reference to the receiver, now holding the samples. The write is
    in-place and preserves storage identity, so every alias of the receiver
    sees the new values.

## Raises

- if the receiver's dtype is not floating-point or complex
- if `mean` or `stddev` is non-finite, or `stddev < 0`

## Gradient

Not differentiable. A random overwrite has no derivative with respect to the
values it replaced or to `mean` / `stddev`.

## Examples

Example 1. Fill a buffer with standard-normal samples. Seeded here, so the
values are fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({5}, pluss::ScalarType::Float64);
x.normal_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.4417, 0.3803, 0.3206, 1.6263, -1.0114]
```

Example 2. The same seed gives the same draw; without reseeding, the next
call draws fresh values.

```cpp
pluss::rng_seed(7);
auto a = pluss::series::empty({4}, pluss::ScalarType::Float64); a.normal_();
pluss::rng_seed(7);
auto b = pluss::series::empty({4}, pluss::ScalarType::Float64); b.normal_();
auto c = pluss::series::empty({4}, pluss::ScalarType::Float64); c.normal_();  // stream advanced
std::cout << std::boolalpha
          << "a == b (reseeded):     " << a.equal(b) << "\n"
          << "a == c (not reseeded): " << a.equal(c) << "\n";
```

### Output

```text
a == b (reseeded):     true
a == c (not reseeded): false
```

Example 3. `mean` and `stddev` place and scale the samples.

```cpp
pluss::rng_seed(1);
auto x = pluss::series::empty({5}, pluss::ScalarType::Float64);
x.normal_(100.0, 0.1);
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[100.1179, 100.0694, 99.9451, 99.9794, 100.0126]
```

Every value sits within a few tenths of `100` — `stddev = 0.1`.

Example 4. Over a large sample, the empirical moments converge to the
requested ones.

```cpp
pluss::rng_seed(2024);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Float64);
x.normal_(3.0, 2.0);
std::cout << "mean~" << x.mean().to_vec<double>()[0]
          << "  stddev~" << x.std().to_vec<double>()[0] << "\n";
```

### Output

```text
mean~3.00064  stddev~2.0055
```

A finite sample never matches exactly; the spread of these estimates
shrinks like `1/sqrt(n)`.

Example 5. `stddev = 0` is a valid degenerate fill — every element becomes
`mean`.

```cpp
auto x = pluss::series::empty({3}, pluss::ScalarType::Float64);
x.normal_(5.0, 0.0);
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5.0000, 5.0000, 5.0000]
```

Example 6. An integer receiver is rejected; so is a negative `stddev`.

```cpp
try {
  auto i = pluss::series::empty({3}, pluss::ScalarType::Int64);
  i.normal_();
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.normal_(0.0, -1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
normal_: receiver must have a floating-point or complex dtype
normal_: expected finite parameters with std >= 0
```

For an integer buffer of random values, use [`random_`](random_.md).

Example 7. The fill is in-place — every alias of the receiver sees it.

```cpp
pluss::rng_seed(9);
auto base = pluss::series::empty({3}, pluss::ScalarType::Float64);
auto alias = base;
base.normal_();
std::cout << std::boolalpha << "alias.is_set_to(base): " << alias.is_set_to(base) << "\n";
std::cout << "alias = ";
pluss::series::print(std::cout, alias);
```

### Output

```text
alias.is_set_to(base): true
alias = Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.9341, 1.0008, 1.9234]
```

The factory [`randn`](../series_factory/randn.md) would instead return a new
tensor and leave `base` untouched.

## Complex Receivers

For a `Complex64` / `Complex128` receiver, the real and imaginary parts are
drawn independently, each with standard deviation `stddev / sqrt(2)`, so the
expected squared distance from the (real) `mean` is still `stddev * stddev`.
`mean` shifts the real component; the imaginary component stays centered at
zero.

## Gotchas And Failure Modes

- The second parameter is **standard deviation**, not variance.
- `normal_` fills an **existing** shape — it does not build one from the
  parameters. Use [`randn`](../series_factory/randn.md) /
  [`normal`](../series_factory/normal.md) to create.
- The receiver dtype must be floating-point or complex; integers are
  rejected (use [`random_`](random_.md)).
- Draws are reproducible only if you set `pluss::rng_seed(seed)` first; each
  call then advances the stream.
- Not differentiable — a `tracks_slope_` receiver gains nothing from being
  overwritten with noise.

## Summary

`normal_(mean = 0, stddev = 1)` fills the receiver in place with Gaussian
samples:

- receiver supplies shape/dtype/platform/storage; old values are discarded
- second argument is **standard deviation**; `stddev = 0` gives a constant
  fill
- reproducible under `pluss::rng_seed(seed)`; each call advances the stream
- floating-point or complex dtype only; not differentiable; aliases see the
  write

## See Also

- [`pluss::series::randn`](../series_factory/randn.md) /
  [`pluss::series::normal`](../series_factory/normal.md): create a Gaussian
  tensor instead of filling one.
- [`Series::uniform_`](uniform_.md): bounded continuous fill.
- [`Series::log_normal_`](log_normal_.md): fill with `exp` of normal samples
  (positive, right-skewed).
- [`Series::cauchy_`](cauchy_.md): a heavy-tailed location-scale alternative.
- [`Series::random_`](random_.md): integer-valued fill.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="norm.html">
    <span class="label">Previous</span>
    <span class="title">norm</span>
  </a>
  <a class="next-link" href="numel.html">
    <span class="label">Next</span>
    <span class="title">numel</span>
  </a>
</div>
