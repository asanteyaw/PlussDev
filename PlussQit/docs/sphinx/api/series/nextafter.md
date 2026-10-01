<!-- pluss-hand-authored -->

# Series::nextafter

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::nextafter(const Series& other) const -> Series
Series& Series::nextafter_(const Series& other)
Series& Series::nextafter_(const Series& rhs) const
```

## What It Does

`nextafter()` returns the next representable floating-point value after the
receiver `a`, stepping **toward** `other` `b`.

The step is exactly one ULP (unit in the last place) — the smallest change the
format can express at that magnitude. If `a == b`, the result is `b` unchanged.

This is a low-level tool. It is used to build a strict bound from a
non-strict one (the smallest value strictly greater than `x` is
`nextafter(x, +inf)`), to nudge a value off an exact boundary before a
comparison, or to enumerate the representable grid near a point when testing
numerical code.

## Parameters

**other** (`const Series&`)

The direction to step. Only whether `b` is greater than, less than, or equal to
`a` matters. Broadcasts against the receiver.

## Returns

`Series` or `Series&`
    `a` advanced one representable step toward `b`. `nextafter_()` writes the
    result back into the receiver.

## Gradient

`nextafter()` is not differentiable. It is a discrete step along the
representable grid.

## In-Place Form

`nextafter_()` is the in-place sibling of [`nextafter()`](nextafter.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `nextafter_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0}, {1});
auto y = pluss::make_series({2.0}, {1});

x.nextafter_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1]
```

## Examples

Example 1. Stepping up from `1.0` and down from `1.0`. The change is one ULP,
about `2.2e-16` at this magnitude, so a rounded printout still shows `1`.

```cpp
auto a = pluss::make_series({1.0, 1.0});
auto b = pluss::make_series({2.0, 0.0});

std::cout << a.nextafter(b);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

The two results differ from `1.0` and from each other in the last bit; the
default formatting rounds both back to `1`.

Example 2. Stepping from `0.0` toward a negative number lands on the smallest
representable magnitude — a subnormal.

```cpp
std::cout << pluss::make_series({0.0}).nextafter(pluss::make_series({-1.0}));
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[-4.9406565e-324]
```

Example 3. Equal arguments leave the value unchanged.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({3.0}).nextafter(pluss::make_series({3.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3.0000]
```

## Gotchas And Failure Modes

- The step size depends on magnitude. Near `1.0` it is about `2.2e-16`; near
  `1e300` it is about `1e284`. `nextafter()` is not "add a small constant."
- Only the direction of `other` is used, never its magnitude.
- Not differentiable.
- Floating-point inputs.

## Summary

`nextafter(a, b)` is `a` moved one ULP toward `b`:

- one representable step; `a == b` returns `b`
- only the sign of `b - a` matters
- not differentiable

## See Also

- [`Series::clamp_min`](clamp_min.md) / [`Series::clamp_max`](clamp_max.md): for
  bounding by a value rather than nudging by one ULP.
- [`Series::isfinite`](isfinite.md): checking the result stayed in range after a
  sequence of steps.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="neg.html">
    <span class="label">Previous</span>
    <span class="title">neg</span>
  </a>
  <a class="next-link" href="no_grad.html">
    <span class="label">Next</span>
    <span class="title">no_grad</span>
  </a>
</div>
