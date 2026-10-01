<!-- pluss-hand-authored -->

# Series::lerp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::lerp(const Series& end, const Series& weight) const -> Series
Series& Series::lerp_(const Series& end, const Series& weight)
Series& Series::lerp_(const Series& end, double weight) const
Series& Series::lerp_(const Series& end, const Series& weight) const
```

## What It Does

`lerp()` is the linear interpolation between the receiver `start` and `end`,
controlled by `weight`:

```
lerp(start, end, weight) = start + weight * (end - start)
                         = (1 - weight) * start + weight * end
```

- `weight = 0` returns `start`
- `weight = 1` returns `end`
- `weight = 0.5` returns the midpoint
- `weight` outside `[0, 1]` extrapolates past the endpoints; it is not clamped

All three arguments broadcast against each other, so `weight` can be a single
scalar, one value per element, or any broadcast-compatible shape.

## Parameters

**end** (`const Series&`)

The `weight = 1` endpoint. Broadcasts against `start`.

**weight** (`const Series&`)

The interpolation fraction. Broadcasts against `start` and `end`. Not clamped.

## Returns

`Series` or `Series&`
    The interpolated values. `lerp_()` writes the result back into the receiver
    `start`.

## Gradient

`lerp()` is differentiable in all three arguments:

```
d/dstart  lerp = 1 - weight
d/dend    lerp = weight
d/dweight lerp = end - start
```

The `start` and `end` gradients are the two interpolation weights and sum to
`1`; the `weight` gradient is the gap being interpolated across.

## In-Place Form

`lerp_()` is the in-place sibling of [`lerp()`](lerp.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `lerp_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 0.0, 0.0});
auto end = pluss::make_series({10.0, 10.0, 10.0});

x.lerp_(end, 0.5);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 5, 5]
```

```cpp
auto x = pluss::make_series({0.0, 0.0}, {2});
auto end = pluss::make_series({10.0, 20.0}, {2});
auto weight = pluss::make_series({0.5, 0.25}, {2});

x.lerp_(end, weight);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 5]
```

## Examples

Example 1. Sweeping `weight` from `0` to `1` walks from `start` to `end`.

```cpp
auto start = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0});
auto end   = pluss::make_series({10.0, 10.0, 10.0, 10.0, 10.0});
auto weight = pluss::make_series({0.0, 0.25, 0.5, 0.75, 1.0});

pluss::series::print(std::cout, start.lerp(end, weight));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0000, 2.5000, 5.0000, 7.5000, 10.0000]
```

Example 2. A scalar `weight` broadcasts to every element.

```cpp
auto start = pluss::make_series({0.0, 2.0, 4.0});
auto end   = pluss::make_series({10.0, 10.0, 10.0});

pluss::series::print(std::cout, start.lerp(end, pluss::make_series(0.5)));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5.0000, 6.0000, 7.0000]
```

Example 3. `weight` outside `[0, 1]` extrapolates rather than clamping.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0}).lerp(pluss::make_series({10.0}), pluss::make_series({1.5})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[15.0000]
```

Example 4. The gradient is `(1 - weight, weight, end - start)`.

```cpp
auto start = pluss::make_series({1.0, 2.0});
auto end   = pluss::make_series({5.0, 8.0});
auto weight = pluss::make_series({0.25, 0.75});
start.tracks_slope_(true);
end.tracks_slope_(true);
weight.tracks_slope_(true);

start.lerp(end, weight).sum().reverse();

std::cout << "dstart="  << start.sensitivity()  << "\n";
std::cout << "dend="    << end.sensitivity()    << "\n";
std::cout << "dweight=" << weight.sensitivity() << "\n";
```

### Output

```text
dstart=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.75, 0.25]
dend=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.25, 0.75]
dweight=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
```

`dstart + dend = 1` everywhere, and `dweight` is `end - start`, which is `4`
and `6`.

## Gotchas And Failure Modes

- `weight` is not clamped. Values outside `[0, 1]` extrapolate, which is
  sometimes wanted and sometimes a bug; clamp `weight` yourself if you need the
  result bounded by the endpoints.
- The receiver is `start`, the first explicit argument is `end`. Swapping them
  reverses the interpolation direction.
- `lerp_()` overwrites `start`.

## Summary

`lerp(start, end, weight)` is `start + weight * (end - start)`:

- all three arguments broadcast; `weight` is not clamped
- differentiable in all three, gradients `(1 - weight, weight, end - start)`

## See Also

- [`Series::add`](add.md), [`Series::sub`](sub.md), [`Series::mul`](mul.md): the
  arithmetic `lerp()` packages into one differentiable step.
- [`Series::clamp_min`](clamp_min.md) / [`Series::clamp_max`](clamp_max.md): to
  bound `weight` before interpolating if extrapolation is unwanted.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="leaky_relu.html">
    <span class="label">Previous</span>
    <span class="title">leaky_relu</span>
  </a>
  <a class="next-link" href="lgamma.html">
    <span class="label">Next</span>
    <span class="title">lgamma</span>
  </a>
</div>
