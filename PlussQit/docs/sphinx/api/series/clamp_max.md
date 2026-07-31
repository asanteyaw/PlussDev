<!-- pluss-hand-authored -->

# Series::clamp_max

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::clamp_max(double max) const -> Series
Series& Series::clamp_max_(double max)
Series::clamp_max(const Series& max) const -> Series
Series& Series::clamp_max_(const Series& max)
```

## What It Does

Caps every element from above: `out[i] = self[i] <= max ? self[i] : max` — a
one-sided `clamp` (two-sided min+max in one call) that only ever pulls values
*down*, never up. The
`_` suffix pair mutate `self` in place instead of returning a new `Series`.

## Parameters

**max** (`double` or `const Series&`)

The upper bound. For the `double` overload, every element is capped at the same
value. For the `Series` overload, the bound broadcasts elementwise against `self`.

## Returns

`Series` (or `Series&` for the `_` variants)
    A new `Series` (or `self`, mutated) with every element `<= max`.

## Gradient

Both overloads are differentiable. `d(out)/d(self[i]) = 1` where `self[i] <= max`,
else `0` — an **inclusive** boundary, matching
[`maximum`](maximum.md)/[`minimum`](minimum.md)'s own tie-breaking convention. For
the `Series` overload, `max` gets the complementary gradient: `1` exactly where
`self[i] > max` (see Example 4).

```{note} Why the boundary is inclusive
This inclusive choice isn't arbitrary: `elu(x) = relu(x) + alpha * (exp(clamp_max(x, 0)) - 1)`
needs `clamp_max`'s derivative at `x == 0` to be exactly `1` (matching
`relu()`'s own derivative there) for `elu`'s combined derivative to be
continuous at the boundary. An exclusive `x < max` boundary would make `clamp_max`'s
contribution `0` at `x == 0` instead, producing a real (if easy to miss) kink.
```

The in-place `_` variant is **not** independently differentiable — like
`relu_()`, in-place ops in this engine are treated as low-level storage
mutations; use the out-of-place `clamp_max()` inside anything that needs to
backpropagate through the clamp itself.

## Examples

Example 1. Cap a mix of negative, zero, and positive values at `0.0`.

```cpp
auto x = pluss::make_series(std::vector<double>{-2.0, -0.5, 0.0, 1.0, 3.0}, {5});
std::cout << x.clamp_max(0.0);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0, 0]
```

Everything at or below `0.0` passes through unchanged; `1.0` and `3.0` are pulled
down to the cap.

Example 2. Gradient at the boundary.

```cpp
auto x = pluss::make_series(std::vector<double>{-2.0, -0.5, 0.0, 1.0, 3.0}, {5});
x.tracks_slope_(true);

auto y = x.clamp_max(0.0);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2, -0.5, 0, 0, 0]
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 1, 1, 0, 0]
```

`x == 0.0` (index 2) gets gradient `1`, not `0` — the inclusive boundary described
above.

Example 3. The `Series`-argument overload: each element is capped at its own,
independent bound.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
auto hi = pluss::make_series(std::vector<double>{2, 2, 2}, {3});
std::cout << x.clamp_max(hi);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
```

`1` is already `<= 2`, so it passes through; `5` and `3` are both capped down to
their bound.

Example 4. Gradient of the `Series`-argument overload: it flows to `self` where
`self` was kept, and to `max` where the bound was used instead.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
auto hi = pluss::make_series(std::vector<double>{2, 2, 2}, {3});
x.tracks_slope_(true);
hi.tracks_slope_(true);

auto y = x.clamp_max(hi);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
std::cout << "dhi=" << hi.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
dhi=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1]
```

Index 0 (`x=1 <= hi=2`, `x` passed through): gradient goes to `x`. Indices 1 and 2
(`x > hi`, the bound was used): gradient goes to `hi` instead. Every position's
gradient lands on exactly one of the two operands, summing to `1` either way.

## Notes

- `clamp_max`'s inclusive boundary at `self[i] == max` is a deliberate choice tied
  to `elu()`'s derivative continuity, not an incidental default — see the Gradient
  section.
- For a two-sided bound (both floor and ceiling in one call), see `clamp(min, max)`.

## See Also

- [`Series::clamp_min`](clamp_min.md): the mirrored floor.
- [`Series::minimum`](minimum.md): semantically equivalent to the `Series`-argument
  overload (`self.clamp_max(hi) == self.minimum(hi)`), though implemented
  separately via `where()` rather than sharing code with it.
- `Series::clamp(min, max)`: both bounds in a single call (not yet documented on
  its own page).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_put_.html">
    <span class="label">Previous</span>
    <span class="title">index_put_</span>
  </a>
  <a class="next-link" href="clamp_min.html">
    <span class="label">Next</span>
    <span class="title">clamp_min</span>
  </a>
</div>
