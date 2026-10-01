<!-- pluss-hand-authored -->

# Series::rrelu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::rrelu(
    double lower = 0.125,
    double upper = 0.3333333333333333,
    bool training = false) const
Series& Series::rrelu_(bool training) const
```

## Randomized Or Deterministic Negative Slopes

`rrelu()` leaves positive elements unchanged and multiplies each non-positive
element by a slope. The source of that slope depends on `training`:

- with `training=false`, every non-positive lane uses the deterministic
  midpoint `(lower + upper) / 2`;
- with `training=true`, every non-positive lane receives its own independent
  slope sampled uniformly from the closed numeric interval bounded by `lower`
  and `upper`.

Evaluation mode is therefore an ordinary leaky rectifier with a fixed slope.
Training mode injects multiplicative noise only into the non-positive branch.
This split lets training use randomized slopes while inference remains stable
and reproducible.

`lower` and `upper` must be finite and ordered, but they need not be positive.
A negative interval is meaningful: multiplying a negative input by a negative
slope produces a positive output. Most activation use chooses a non-negative
interval, but the method enforces ordering rather than imposing that policy.

## Shape, Dtype, And Randomness

- The result has the same shape, dtype, and platform as the receiver.
- The receiver must have a floating-point dtype. Integer, Boolean, and complex
  receivers are rejected rather than silently changing the sampling domain.
- Positive lanes do not consume their sampled slope semantically; their output
  and derivative are exactly the input and one.
- The method is out of place and does not mutate the receiver.
- Set the shared Pluss random seed before a training call when reproducible
  examples or experiments are required. Re-seeding resets the stream.

## Reverse Mode

The exact slopes realized during the training forward pass are retained by its
single reverse node. A negative lane therefore receives the same slope in
reverse; backward never resamples. Evaluation mode uses the deterministic
midpoint in both directions. At zero the non-positive branch applies.

## In-Place Form

`rrelu_()` is the in-place sibling of [`rrelu()`](rrelu.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `rrelu_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});

// training=false uses the deterministic eval-mode slope
// (the midpoint of [lower, upper]), not a random draw.
x.rrelu_(false);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.45833333, 0, 2]
```

## Examples

### 1. Use deterministic defaults for evaluation

```cpp
const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
pluss::series::print(std::cout, x.rrelu());
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.6875, -0.2292, 0.0000, 1.0000, 3.0000]
```

The default midpoint is `(0.125 + 1/3) / 2`, approximately `0.2291667`.
Because `training` defaults to `false`, repeated calls produce the same values.

### 2. Choose a custom evaluation interval

```cpp
const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
pluss::series::print(std::cout, x.rrelu(0.2, 0.4, false));
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-0.9000, -0.3000, 0.0000, 1.0000, 3.0000]
```

Evaluation uses midpoint `0.3`; it does not draw a random value from the
interval.

### 3. Ordered negative bounds are valid

```cpp
const auto x = pluss::make_series({-2.0, -1.0, 0.0, 2.0});
pluss::series::print(std::cout, x.rrelu(-0.4, -0.2, false));
```

#### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.6000, 0.3000, 0.0000, 2.0000]
```

The midpoint is `-0.3`. This example is mathematically valid, although a
positive negative-branch slope is the usual activation policy.

### 4. Seed a training-mode draw

```cpp
pluss::rng_seed(20260928);
const auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
pluss::series::print(std::cout, x.rrelu(0.2, 0.4, true));
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1.0932, -0.2805, 0.0000, 1.0000, 3.0000]
```

Each of the first three lanes receives an independent slope in `[0.2, 0.4]`.
The positive lanes remain exact identities.

### 5. Reverse mode reuses the realized slopes

```cpp
pluss::rng_seed(20260928);
auto x = pluss::make_series({-3.0, -1.0, 0.0, 1.0, 3.0});
x.tracks_slope_(true);
x.rrelu(0.2, 0.4, true).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.3644, 0.2805, 0.2320, 1.0000, 1.0000]
```

For example, `-3 * 0.3644` gives the first forward value `-1.0932` after
rounding. The reverse slope is the same draw, including at zero where the
output alone cannot reveal it.

### 6. Reject reversed bounds

```cpp
try {
  const auto x = pluss::make_series({-1.0, 1.0});
  (void)x.rrelu(0.5, 0.1, true);
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
rrelu: expected finite bounds with lower <= upper
```

NaN and infinite bounds fail the same finite-interval validation.

### 7. Reject non-floating receivers

```cpp
try {
  (void)pluss::make_series({-2, 1}).rrelu();
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
rrelu: input must have a floating-point dtype
```

Explicitly convert an integer Series when floating randomized activation is
intended.

## Performance

Evaluation mode is a deterministic elementwise activation. Training mode also
generates one slope per lane and retains that slope Series for reverse mode.
It is therefore more expensive in both bandwidth and saved state. Do not use
`training=true` for inference when the randomized behavior is unwanted.

## See Also

- [`leaky_relu()`](leaky_relu.md) accepts one deterministic negative slope.
- [`relu()`](relu.md) uses a zero negative slope.
- [`uniform_()`](uniform_.md) fills an existing Series from a uniform
  distribution.
