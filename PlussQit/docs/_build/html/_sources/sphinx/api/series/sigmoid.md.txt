<!-- pluss-hand-authored -->

# Series::sigmoid

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::sigmoid() const -> Series
Series& Series::sigmoid_()
```

## What It Does

`sigmoid()` applies the logistic map elementwise:

`σ(x) = 1 / (1 + e^{-x})`

Every real input is squeezed into the open interval `(0, 1)`.

That gives `sigmoid()` a very different feel from [`relu()`](relu.md): it never
produces a hard zero-or-pass gate, and it never exceeds `1`.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, containing the logistic value
    for each input lane.

## Gradient

If `y = σ(x)`, then:

- `dy/dx = σ(x) (1 - σ(x))`

So the derivative is largest around `x = 0` and shrinks toward the saturated
ends.

## In-Place Form

`sigmoid_()` is the in-place sibling of [`sigmoid()`](sigmoid.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({0.0, 1.0});

auto& ref = x.sigmoid_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 0.73105858]
same object: true
```

## Examples

Example 1. Large negative input lands near `0`, zero lands at `0.5`, and large
positive input lands near `1`.

```cpp
std::cout << pluss::series::print(pluss::make_series({-2.0, 0.0, 2.0}).sigmoid()) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.1192, 0.5000, 0.8808]
```

Example 2. The reverse pass multiplies by `σ(x)(1 - σ(x))`.

```cpp
auto x = pluss::make_series({-2.0, 0.0, 2.0});
x.tracks_slope_(true);

auto y = x.sigmoid();
y.sum().reverse();

std::cout << "out=" << y << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.11920292, 0.5, 0.88079708]
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.10499359, 0.25, 0.10499359]
```

## Gotchas And Failure Modes

- `sigmoid()` never hard-clips, but it does saturate toward `0` and `1`.
- Because the derivative is `σ(x)(1 - σ(x))`, the slope is strongest near the
  middle and weaker near the tails.
- There are no axis toggles or broadcasting branches here; it is purely
  elementwise.

## Summary

Use `sigmoid()` when each lane should be mapped smoothly into `(0, 1)`.

## See Also

- [`Series::relu`](relu.md): hard rectification instead of smooth squashing.
- [`Series::tanh`](tanh.md): another smooth elementwise nonlinearity with a
  different output range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sgn.html">
    <span class="label">Previous</span>
    <span class="title">sgn</span>
  </a>
  <a class="next-link" href="sign.html">
    <span class="label">Next</span>
    <span class="title">sign</span>
  </a>
</div>
