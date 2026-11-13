<!-- pluss-hand-authored -->

# Series::relu

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::relu() const
Series& Series::relu_()
```

## What It Does

`relu()` applies the rectified linear unit elementwise:

- if `x > 0`, keep `x`
- if `x <= 0`, return `0`

So it is a piecewise-linear gate. Negative lanes are shut off, positive lanes
pass through unchanged.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, containing the rectified
    result lane by lane.

## Gradient

For `y = relu(x)`, the local slope is piecewise:

- `1` when `x > 0`
- `0` when `x < 0`

At `x = 0`, the current verified surface also uses `0`. That convention is
visible directly in the example below.

## In-Place Form

`relu_()` is the in-place sibling of [`relu()`](relu.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 2.0});

auto& ref = x.relu_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 2]
same object: true
```

## Examples

Example 1. Negative and zero lanes clamp to zero, while positive lanes survive.

```cpp
std::cout << pluss::make_series({-2.0, 0.0, 3.0}).relu() << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 3]
```

Example 2. Reverse-mode follows the same gate: off lanes get zero sensitivity,
active lanes get one.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 3.0});
x.tracks_slope_(true);

auto y = x.relu();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 3]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 1]
```

## Gotchas And Failure Modes

- `relu()` is not smooth at zero.
- On the current verified surface, the stored slope at exactly zero is `0`.
- This is an elementwise gate, not a reduction or threshold-counting method.

## Summary

Use `relu()` when negative lanes should be suppressed and positive lanes should
pass through unchanged.

## See Also

- [`Series::sigmoid`](sigmoid.md): smooth bounded gate instead of hard
  rectification.
- [`Series::clamp_min`](clamp_min.md): lower-bound clipping with a configurable
  floor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reciprocal.html">
    <span class="label">Previous</span>
    <span class="title">reciprocal</span>
  </a>
  <a class="next-link" href="remainder.html">
    <span class="label">Next</span>
    <span class="title">remainder</span>
  </a>
</div>
