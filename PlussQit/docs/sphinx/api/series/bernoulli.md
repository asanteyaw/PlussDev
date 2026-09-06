<!-- pluss-hand-authored -->

# Series::bernoulli

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::bernoulli() const
Series Series::bernoulli(double p) const
```

## What It Does

Returns a **new** `Series` of independent `0`/`1` outcomes — one coin flip
per element — without touching the receiver. (The in-place counterpart is
[`bernoulli_`](bernoulli_.md).)

The two overloads answer different questions:

- **`bernoulli()`** — every receiver element *is its own success
  probability*. The receiver must be floating-point and every value must be
  finite and in `[0, 1]`; invalid probabilities are rejected, not clipped.
  Use this when a probability model has already produced a `Series`.
- **`bernoulli(p)`** — the receiver values are **ignored**; it contributes
  only the output shape, dtype, and platform, and every position uses the
  same `p`. Use this when an existing `Series` is just the shape you want.
  This overload (like `bernoulli_`) also accepts integer and `Bool`
  receivers — only complex is rejected.

The result keeps the receiver's dtype — it is not a `Bool` mask (unless the
receiver was already `Bool`).

### Reproducibility

Deterministic given the process-wide RNG seed. Call `pluss::rng_seed(seed)`
before sampling to fix the outcomes; two runs from the same seed produce
`equal` tensors. Each call advances a stream counter.

## Parameters

**p** (`double`, for the one-argument form): the shared success probability.
Must be finite and in `[0, 1]`. `p = 0` fills with zeros, `p = 1` with ones.

## Returns

`Series`
    A new tensor of `0`/`1` outcomes with the receiver's shape, dtype, and
    platform. The receiver is unchanged.

## Raises

- `bernoulli()`: if the receiver is not floating-point, or any element is
  non-finite or outside `[0, 1]`
- `bernoulli(p)`: if the receiver dtype is complex, or `p` is non-finite or
  outside `[0, 1]`

## Gradient

Not differentiable — the outcomes carry no derivative back to the
probabilities.

## Examples

Example 1. `bernoulli()` — the receiver values are the per-element
probabilities.

```cpp
auto p = pluss::make_series({0.0, 1.0, 0.0, 1.0});
pluss::series::print(std::cout, p.bernoulli());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.0000, 0.0000, 1.0000]
```

Probability `0` can never succeed and probability `1` can never fail, so this
draw is deterministic.

Example 2. `bernoulli(p)` ignores the receiver values — it uses only the
shape and dtype.

```cpp
pluss::rng_seed(42);
auto canvas = pluss::series::zeros({10}, pluss::ScalarType::Float64);
auto out = canvas.bernoulli(0.5);
std::cout << "canvas unchanged: "; pluss::series::print(std::cout, canvas);
std::cout << "out = "; pluss::series::print(std::cout, out);
```

### Output

```text
canvas unchanged: Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000]out = Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0.0000, 1.0000, 1.0000, 1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000]
```

The ten `0.0`s in `canvas` did not force failures — this overload uses
`0.5`, not the receiver's values — and `canvas` itself is untouched.

Example 3. `bernoulli()` diagnoses out-of-range probabilities before
producing anything.

```cpp
try {
  pluss::make_series({-0.1, 0.5}).bernoulli();
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  pluss::make_series({0.5, 1.2}).bernoulli();
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
bernoulli: probabilities must be finite and in [0, 1]
bernoulli: probabilities must be finite and in [0, 1]
```

Example 4. Reproducible under a fixed seed; the receiver is never modified.

```cpp
pluss::rng_seed(3);
auto p = pluss::make_series({0.5, 0.5, 0.5, 0.5});
auto a = p.bernoulli();
pluss::rng_seed(3);
auto b = p.bernoulli();
std::cout << std::boolalpha << "a == b: " << a.equal(b) << "  p unchanged: ";
pluss::series::print(std::cout, p);
```

### Output

```text
a == b: true  p unchanged: Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5000, 0.5000, 0.5000, 0.5000]
```

## Gotchas And Failure Modes

- `bernoulli()` **reads** the receiver as probabilities; `bernoulli(p)`
  **ignores** them. Pick the overload deliberately.
- `bernoulli()` requires a floating-point receiver with every value in
  `[0, 1]`; `NaN`, infinity, and out-of-range values fail up front — they
  are not clipped.
- The result keeps the receiver dtype — it is not a `Bool` mask.
- Use [`bernoulli_`](bernoulli_.md) instead only when overwriting the
  receiver is the intent.
- Reproducible only after `pluss::rng_seed(seed)`; each call advances the
  stream.
- Not differentiable.

## Summary

`bernoulli()` / `bernoulli(p)` returns a new `0`/`1` tensor:

- `bernoulli()` uses each receiver element as its own probability
  (floating-point, values in `[0, 1]`)
- `bernoulli(p)` ignores receiver values, uses shared `p`, accepts integer /
  `Bool` / floating receivers
- receiver is never modified; result keeps its dtype
- reproducible under `pluss::rng_seed(seed)`; not differentiable

## See Also

- [`Series::bernoulli_`](bernoulli_.md): the in-place form — fills the
  receiver instead of returning a new tensor.
- [`Series::multinomial`](multinomial.md): draw category indices from
  nonnegative weights.
- [`Series::random_`](random_.md): uniform integers over a range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="beta.html">
    <span class="label">Previous</span>
    <span class="title">beta</span>
  </a>
  <a class="next-link" href="bernoulli_.html">
    <span class="label">Next</span>
    <span class="title">bernoulli_</span>
  </a>
</div>
