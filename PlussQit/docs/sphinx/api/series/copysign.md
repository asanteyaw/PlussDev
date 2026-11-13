<!-- pluss-hand-authored -->

# Series::copysign

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::copysign(const Series& other) const
Series& Series::copysign_(const Series& other)
Series& Series::copysign_(const Series& rhs) const
```

## What It Does

`copysign()` builds a value from the **magnitude** of the receiver `a` and the
**sign** of `other` `b`:

```
copysign(a, b) = |a| * sign(b)
```

So `copysign(5, -3)` is `-5`, and `copysign(-5, 3)` is `5`. The magnitude of
`b` is irrelevant; only whether `b` is positive or negative (including the sign
of a zero) matters.

This is the tool for "give me `x` but pointing the same way as `y`" without a
branch — reflecting a step to move toward a target, matching the orientation of
a normal vector, or restoring a sign that was stripped by an [`abs()`](abs.md).

## Parameters

**other** (`const Series&`)

The sign source `b`. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    `|a|` carrying the sign of `b`. Integer inputs stay integer.
    `copysign_()` writes the result back into the receiver.

## Gradient

`copysign()` is differentiable in the receiver only:

```
d/da copysign(a, b) = sign(a) * sign(b)
d/db copysign(a, b) = 0
```

The receiver's gradient is `+1` when `a` and `b` already share a sign and `-1`
when they differ (that is exactly the factor that maps `a` to `|a| * sign(b)`).
`b` contributes only its sign bit, which carries no usable derivative, so its
gradient is `0`.

## In-Place Form

`copysign_()` is the in-place sibling of [`copysign()`](copysign.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `copysign_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({3.0, -3.0}, {2});
auto y = pluss::make_series({-1.0, 1.0}, {2});

x.copysign_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-3, 3]
```

## Examples

Example 1. Magnitude from `a`, sign from `b`.

```cpp
auto a = pluss::make_series({5.0, -5.0, 5.0});
auto b = pluss::make_series({-3.0, 3.0, 0.0});

pluss::series::print(std::cout, a.copysign(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-5.0000, 5.0000, 5.0000]
```

A positive-zero `b` counts as a positive sign, so the third result is `+5`.

Example 2. Integer inputs stay integer.

```cpp
auto a = pluss::make_series({5, -5, 5}, pluss::ScalarType::Int64);
auto b = pluss::make_series({-1, 1, -1}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, a.copysign(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[-5, 5, -5]
```

Example 3. The gradient flows only to the magnitude source, with sign
`sign(a) * sign(b)`.

```cpp
auto a = pluss::make_series({5.0, -5.0});
auto b = pluss::make_series({-3.0, 3.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.copysign(b).sum().reverse();

std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
da=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1, -1]
db=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
```

For `(a = 5, b = -3)` the output is `-5`, so the derivative wrt `a` is `-1`.
For `(a = -5, b = 3)` the output is `5`, again `-1`. `b` gets nothing.

## Gotchas And Failure Modes

- The magnitude of `other` is ignored. `copysign(5, -0.001)` and
  `copysign(5, -1000)` both give `-5`.
- The sign of a zero in `other` matters: `+0.0` is a positive sign, `-0.0` is
  negative.
- `b` receives no gradient. If you need the target's value to influence
  training, `copysign()` is the wrong op.
- `NaN` in `a` propagates as `NaN` with `b`'s sign; a `NaN` in `b` has a
  platform-defined sign bit.

## Summary

`copysign(a, b)` is `|a|` wearing the sign of `b`:

- broadcasts like `add()`; preserves integer dtype
- only the sign of `b` is read, not its magnitude
- differentiable in `a` (gradient `sign(a) * sign(b)`), zero in `b`

## See Also

- [`Series::abs`](abs.md): drop the sign; `copysign()` is how you put one back.
- [`Series::sign`](sign.md): extract just the sign as `-1 / 0 / +1`.
- [`Series::mul`](mul.md): if you want the magnitude of `b` to matter too.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copy_.html">
    <span class="label">Previous</span>
    <span class="title">copy_</span>
  </a>
  <a class="next-link" href="corrcoef.html">
    <span class="label">Next</span>
    <span class="title">corrcoef</span>
  </a>
</div>
