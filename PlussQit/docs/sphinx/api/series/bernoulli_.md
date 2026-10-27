<!-- pluss-hand-authored -->

# Series::bernoulli_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::bernoulli_() const
Series& Series::bernoulli_(double p) const
```

## What It Does

Overwrites every element of the receiver with an independent `0` or `1` — a
single coin flip per element, with shared success probability `p`. The
no-argument form uses `p = 0.5`.

`p` is **one shared probability for the whole call**. It is *not* read from
the receiver's current values — those are simply discarded. (The
non-mutating [`bernoulli`](bernoulli.md) does support a per-element
probability `Series`; `bernoulli_` does not.)

Like the other in-place samplers, the receiver supplies the shape, dtype,
platform, and storage. The `0`/`1` outcomes are then cast to the receiver's
dtype, so `bernoulli_` accepts **integer and `Bool` receivers as well as
floating-point** — only complex is rejected.

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the outcomes; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**p** (`double`, default `0.5`): the probability of drawing `1`. Must be
finite and in `[0, 1]`. `p = 0` fills with zeros; `p = 1` fills with ones —
both deterministic.

## Returns

`Series&`
    A reference to the receiver, now holding the outcomes. The write is
    in-place and preserves storage identity — every alias sees the values.

## Raises

- if the receiver's dtype is complex
- if `p` is non-finite or outside `[0, 1]`

## Gradient

Not differentiable — there is no derivative with respect to `p`, and the
overwrite discards whatever the receiver held before.

## Examples

Example 1. Default `p = 0.5`. Seeded, so the flips are fixed.

```cpp
pluss::rng_seed(42);
auto x = pluss::series::empty({10}, pluss::ScalarType::Float64);
x.bernoulli_();
pluss::series::print(std::cout, x);
```

### Output

```text
Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0.0000, 1.0000, 1.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000]
```

Example 2. `p` is a shared probability, not read from the buffer. `p = 1`
and `p = 0` are deterministic fills.

```cpp
auto a = pluss::make_series({9.0, 9.0, 9.0}); a.bernoulli_(1.0);
auto b = pluss::make_series({9.0, 9.0, 9.0}); b.bernoulli_(0.0);
std::cout << "p=1: "; pluss::series::print(std::cout, a);
std::cout << "p=0: "; pluss::series::print(std::cout, b);
```

### Output

```text
p=1: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 1.0000, 1.0000]p=0: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.0000, 0.0000]
```

The `9.0`s were ignored — `bernoulli_` does not treat them as probabilities.

Example 3. Over a large sample, the fraction of ones converges to `p`.

```cpp
pluss::rng_seed(7);
auto x = pluss::series::empty({200000}, pluss::ScalarType::Float64);
x.bernoulli_(0.3);
std::cout << "mean~" << x.mean().host<double>()[0] << "  (p = 0.3)\n";
```

### Output

```text
mean~0.301235  (p = 0.3)
```

Example 4. Integer and `Bool` receivers work too — the outcomes are cast to
the receiver dtype.

```cpp
auto i = pluss::series::empty({6}, pluss::ScalarType::Int32); i.bernoulli_(1.0);
auto bl = pluss::series::empty({6}, pluss::ScalarType::Bool); bl.bernoulli_(1.0);
std::cout << "int32: "; pluss::series::print(std::cout, i);
std::cout << "bool:  "; pluss::series::print(std::cout, bl);
```

### Output

```text
int32: Series(shape=[6], strides=[1], dtype=int32, platform=cpu, numel=6, contiguous=true)
data=[1, 1, 1, 1, 1, 1]bool:  Series(shape=[6], strides=[1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[true, true, true, true, true, true]
```

This is a difference from [`normal_`](normal_.md) / [`uniform_`](uniform_.md),
which require a floating (or complex) dtype.

Example 5. A complex receiver, and a `p` outside `[0, 1]`, are rejected.

```cpp
try {
  auto c = pluss::series::empty({3}, pluss::ScalarType::Complex64);
  c.bernoulli_(0.5);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  auto f = pluss::series::empty({3}, pluss::ScalarType::Float64);
  f.bernoulli_(1.5);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
bernoulli_: complex receivers are not supported
bernoulli_: expected a finite probability in [0, 1]
```

Example 6. Reproducible under a fixed seed, and visible through aliases.

```cpp
pluss::rng_seed(3);
auto a = pluss::series::empty({8}, pluss::ScalarType::Float64); a.bernoulli_(0.5);
pluss::rng_seed(3);
auto b = pluss::series::empty({8}, pluss::ScalarType::Float64);
auto alias = b;
b.bernoulli_(0.5);
std::cout << std::boolalpha
          << "a == b: " << a.equal(b)
          << "  alias.is_set_to(b): " << alias.is_set_to(b) << "\n";
```

### Output

```text
a == b: true  alias.is_set_to(b): true
```

## Gotchas And Failure Modes

- `p` is one shared probability, **not** "read probabilities from this
  `Series`". For a per-element probability tensor, use the non-mutating
  [`bernoulli`](bernoulli.md).
- Accepts integer and `Bool` receivers (unlike the continuous samplers);
  only complex is rejected.
- `p < 0`, `p > 1`, `NaN`, and infinity are rejected.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`bernoulli_(p = 0.5)` fills the receiver in place with independent `0`/`1`
outcomes at shared probability `p`:

- `p` is one number for the whole call, not read from the buffer
- outcomes cast to the receiver dtype — integer, `Bool`, or floating; not
  complex
- reproducible under `pluss::rng_seed(seed)`; each call advances the stream
- not differentiable; aliases see the write

## See Also

- [`Series::bernoulli`](bernoulli.md): the non-mutating form, which also
  accepts a per-element probability `Series`.
- [`Series::random_`](random_.md): uniform integers over an arbitrary range.
- [`Series::geometric_`](geometric_.md): the count of trials up to the first
  success — repeated Bernoulli trials.
- [`Series::uniform_`](uniform_.md): the continuous `[0, 1)` fill `bernoulli_`
  thresholds internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bernoulli.html">
    <span class="label">Previous</span>
    <span class="title">bernoulli</span>
  </a>
  <a class="next-link" href="bincount.html">
    <span class="label">Next</span>
    <span class="title">bincount</span>
  </a>
</div>
