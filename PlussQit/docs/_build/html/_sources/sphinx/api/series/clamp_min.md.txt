<!-- pluss-hand-authored -->

# Series::clamp_min

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::clamp_min(double min) const -> Series
Series& Series::clamp_min_(double min)
Series::clamp_min(const Series& min) const -> Series
Series& Series::clamp_min_(const Series& min)
```

## What It Does

Floors every element from below: `out[i] = self[i] >= min ? self[i] : min` — the
mirror image of [`clamp_max`](clamp_max.md). The `_` suffix pair mutate `self` in
place instead of returning a new `Series`.

## Parameters

**min** (`double` or `const Series&`)

The lower bound. For the `double` overload, every element is floored at the same
value. For the `Series` overload, the bound broadcasts elementwise against `self`.

## Returns

`Series` (or `Series&` for the `_` variants)
    A new `Series` (or `self`, mutated) with every element `>= min`.

## Gradient

Both overloads are differentiable. `d(out)/d(self[i]) = 1` where `self[i] >= min`,
else `0` — an **inclusive** boundary, matching [`clamp_max`](clamp_max.md)'s own
convention and, through it, [`maximum`](maximum.md)/[`minimum`](minimum.md)'s
tie-breaking rule. For the `Series` overload, `min` gets the complementary
gradient: `1` exactly where `self[i] < min` (see Example 4).

The in-place `_` variant is **not** independently differentiable, for the same
reason as `clamp_max_`: in-place ops in this engine are treated as low-level
storage mutations. Use the out-of-place `clamp_min()` inside anything that needs
to backpropagate through the clamp itself.

## Examples

Example 1. Floor a mix of negative, zero, and positive values at `0.0`.

```cpp
auto x = pluss::make_series(std::vector<double>{-2.0, -0.5, 0.0, 1.0, 3.0}, {5});
std::cout << x.clamp_min(0.0);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 0, 1, 3]
```

Everything at or below `0.0` is raised to the floor; `1.0` and `3.0` already clear
it and pass through unchanged. This is exactly how `relu()` is defined in this
engine (`relu(x) == clamp_min(x, 0.0)`).

Example 2. Gradient at the boundary.

```cpp
auto x = pluss::make_series(std::vector<double>{-2.0, -0.5, 0.0, 1.0, 3.0}, {5});
x.tracks_slope_(true);

auto y = x.clamp_min(0.0);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 0, 1, 3]
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 0, 1, 1, 1]
```

`x == 0.0` (index 2) gets gradient `1`, not `0` — the inclusive boundary.

Example 3. The `Series`-argument overload: each element is floored at its own,
independent bound.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
auto lo = pluss::make_series(std::vector<double>{2, 2, 2}, {3});
std::cout << x.clamp_min(lo);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 5, 3]
```

`1` is below its bound of `2`, so it's raised to `2`; `5` and `3` already clear
their bound and pass through unchanged.

Example 4. Gradient of the `Series`-argument overload: it flows to `self` where
`self` was kept, and to `min` where the bound was used instead.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
auto lo = pluss::make_series(std::vector<double>{2, 2, 2}, {3});
x.tracks_slope_(true);
lo.tracks_slope_(true);

auto y = x.clamp_min(lo);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
std::cout << "dlo=" << lo.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 5, 3]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1]
dlo=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
```

Index 0 (`x=1 < lo=2`, the bound was used): gradient goes to `lo`. Indices 1 and 2
(`x >= lo`, `x` passed through): gradient goes to `x` instead — the mirror image of
[`clamp_max`](clamp_max.md)'s Example 4.

## Notes

- `clamp_min(x, 0.0)` is literally how `relu(x)` is defined in this engine.
- `clamp_min`'s inclusive boundary at `self[i] == min` mirrors
  [`clamp_max`](clamp_max.md)'s own — see its Gradient section for why that
  boundary choice matters.

## See Also

- [`Series::clamp_max`](clamp_max.md): the mirrored ceiling.
- [`Series::maximum`](maximum.md): semantically equivalent to the `Series`-argument
  overload (`self.clamp_min(lo) == self.maximum(lo)`), though implemented
  separately via `where()` rather than sharing code with it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clamp_max.html">
    <span class="label">Previous</span>
    <span class="title">clamp_max</span>
  </a>
  <a class="next-link" href="copy_.html">
    <span class="label">Next</span>
    <span class="title">copy_</span>
  </a>
</div>
