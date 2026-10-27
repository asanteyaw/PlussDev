<!-- pluss-hand-authored -->

# Series::geometric_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::geometric_() const
Series& Series::geometric_(double p) const
```

## What It Does

Overwrites every element of the receiver with an independent count from a
geometric distribution: the number of Bernoulli trials, each with success
probability `p`, **up to and including the first success**. The no-argument
form uses `p = 0.5`.

Every sample is a positive integer: `1` means success on the first trial,
`2` means one failure then a success, and so on — support starts at `1`, not
`0` (this convention counts the successful trial). The expected count is
`1 / p`, so small `p` gives a long right tail and a large mean.

The receiver holds the counts as its own dtype (which must be
**floating-point**), so a `Float64` buffer stores mathematically-integral
values like `3.0`. Internally each sample is `floor(ln(1 - U) / ln(1 - p)) + 1`
for a uniform `U` — a transformed [`uniform_`](uniform_.md) fill.

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the counts; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**p** (`double`, default `0.5`): the per-trial success probability. Must be
**strictly** in `(0, 1)` — both endpoints are rejected (`p = 0` never
succeeds; `p = 1` is the degenerate "always 1" case the contract excludes).

## Returns

`Series&`
    A reference to the receiver, now holding the counts. The write is
    in-place and preserves storage identity — every alias sees the values.

## Raises

- if the receiver's dtype is not floating-point
- if `p` is not strictly between `0` and `1`

## Gradient

Not differentiable — no derivative with respect to `p`, and the overwrite
discards the receiver's previous contents.

## Examples

Example 1. Default `p = 0.5` — positive integer counts. Seeded, so fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({10}, pluss::ScalarType::Float64);
x.geometric_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[3.0000, 1.0000, 1.0000, 1.0000, 2.0000, 2.0000, 2.0000, 3.0000, 5.0000, 1.0000]
```

With `p = 0.5` about half the draws are `1`; the `5.0` is the tail showing
up.

Example 2. Over a large sample the mean approaches `1 / p`, and the minimum
is `1`.

```cpp
pluss::rng_seed(7);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Float64);
x.geometric_(0.25);
auto v = x.host<double>();
double lo = 1e300, sum = 0;
for (double e : v) { lo = std::min(lo, e); sum += e; }
std::cout << "min=" << lo << "  mean~" << sum / v.size() << "  (1 / 0.25 = 4)\n";
```

### Output

```text
min=1  mean~3.99705  (1 / 0.25 = 4)
```

Example 3. The receiver must be floating-point, and `p` must be strictly
inside `(0, 1)`.

```cpp
try {
  auto i = pluss::series::empty({3}, pluss::ScalarType::Int64);
  i.geometric_(0.5);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.geometric_(0.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.geometric_(1.0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
geometric_: receiver must have a floating-point dtype
geometric_: expected 0 < p < 1
geometric_: expected 0 < p < 1
```

Example 4. Reproducible under a fixed seed.

```cpp
pluss::rng_seed(5);
auto a = pluss::series::empty({6}, pluss::ScalarType::Float64); a.geometric_(0.4);
pluss::rng_seed(5);
auto b = pluss::series::empty({6}, pluss::ScalarType::Float64); b.geometric_(0.4);
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "\n";
```

### Output

```text
a == b: true
```

## Gotchas And Failure Modes

- Support starts at `1` (the trial-count convention), not `0`.
- `p` must be **strictly** in `(0, 1)` — `0` and `1` both raise.
- Floating-point receiver only; the counts are stored as integral floating
  values (`3.0`, not `3`). An `Int64` buffer is rejected — draw into a
  `Float64` and [`to`](to.md)`(Int64)` if you want integer storage.
- Counts are unbounded above; a small `p` produces a heavy right tail.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`geometric_(p = 0.5)` fills the receiver in place with trials-until-first-
success counts:

- positive integers (support starts at `1`); mean `1 / p`
- `p` strictly in `(0, 1)`; floating-point receiver only
- built by inverse-CDF from a uniform draw
- reproducible under `pluss::rng_seed(seed)`; not differentiable; aliases
  see the write

## See Also

- [`Series::bernoulli_`](bernoulli_.md): a single trial per element — the
  building block `geometric_` counts.
- [`Series::exponential_`](exponential_.md): the continuous analogue —
  waiting time rather than a discrete trial count.
- [`Series::uniform_`](uniform_.md): the uniform fill `geometric_` transforms
  internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ge.html">
    <span class="label">Previous</span>
    <span class="title">ge</span>
  </a>
  <a class="next-link" href="ger.html">
    <span class="label">Next</span>
    <span class="title">ger</span>
  </a>
</div>
