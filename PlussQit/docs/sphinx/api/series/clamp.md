<!-- pluss-hand-authored -->

# Series::clamp

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::clamp(double min_v, double max_v) const
Series& Series::clamp_(double min_v, double max_v)
```

## What It Does

Applies a two-sided bound to every element in one call.

Conceptually, for each position `i`:

- if `self[i] < min_v`, the result is `min_v`
- if `self[i] > max_v`, the result is `max_v`
- otherwise, the result is `self[i]` unchanged

This is the combined form of [`clamp_min()`](clamp_min.md) followed by
[`clamp_max()`](clamp_max.md) — `x.clamp(lo, hi)` and
`x.clamp_min(lo).clamp_max(hi)` produce the identical result, in one call
instead of two. `clamp()` returns a new series; `clamp_()` mutates the
current series in place and returns `Series&`.

```{important} min_v must not exceed max_v
`min_v > max_v` throws immediately, before any element is touched — there is
no implicit swap or silent reinterpretation of the bounds. See Example 4.
```

## Parameters

**min_v** (`double`)

The lower bound. Any element below this is replaced by `min_v`.

**max_v** (`double`)

The upper bound. Any element above this is replaced by `max_v`. Must be
`>= min_v`.

Both overloads take the bounds as plain scalars only — unlike
[`clamp_min()`](clamp_min.md)/[`clamp_max()`](clamp_max.md), `clamp()` does
not currently have a `Series`-bound (per-element/broadcast bound) overload.
When each element needs its own lower and upper bound, compose
`clamp_min(lo_series).clamp_max(hi_series)` instead.

## Returns

`Series` or `Series&`
    The bounded result, either as a fresh value (`clamp()`) or as the
    mutated current series (`clamp_()`).

## Gradient

`clamp()` is differentiable. Gradient flows to `self` only where the element
was left unchanged by the bound — that is, where `min_v <= self[i] <= max_v`
— and is zero everywhere the element was actually clamped.

```{note} Inclusive boundaries on both sides
At `self[i] == min_v` or `self[i] == max_v`, the derivative with respect to
`self[i]` is `1`, not `0` — both boundaries are treated as "inside" the
unclamped region. This matches [`clamp_min()`](clamp_min.md)'s and
[`clamp_max()`](clamp_max.md)'s own inclusive-boundary convention exactly,
which is expected since `clamp()` is defined in terms of them. See Example 2.
```

`clamp_()` is a storage mutation, not a differentiable node — use the
out-of-place `clamp()` when the bound itself needs to sit on the
differentiable path.

## Examples

Example 1. A two-sided bound in one call.

```cpp
auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});

std::cout << line.clamp(-1.0, 2.0);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -1, 0, 2, 2]
```

The two strictly-outside values (`-3` and `5`) get pulled to the nearest
bound; everything already inside `[-1, 2]` (including the boundary value
`-1` itself) passes through unchanged.

Example 2. Both boundaries keep gradient `1`; only the strictly-clamped
positions get gradient `0`.

```cpp
auto x = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
x.tracks_slope_(true);

auto y = x.clamp(-1.0, 2.0);
y.sum().reverse();

std::cout << "y=" << y << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -1, 0, 2, 2]
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0, 1, 1, 1, 0]
```

Position `1` sits exactly at `min_v` and position `3` sits exactly at
`max_v` — both keep derivative `1`. Only positions `0` and `4`, strictly
outside the bounds, get derivative `0`.

Example 3. `clamp_()` mutates the current handle in place.

```cpp
auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});
line.clamp_(-1.0, 2.0);

std::cout << line;
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -1, 0, 2, 2]
```

Example 4. `min_v > max_v` throws instead of silently swapping the bounds.

```cpp
auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});

try {
  auto out = line.clamp(2.0, -1.0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
clamp_: min must be <= max
```

The error is reported under the `clamp_` name even when called through the
out-of-place `clamp()` entry point — both overloads share the same bound
validation.

Example 5. `clamp(lo, hi)` and the composed `clamp_min(lo).clamp_max(hi)`
form agree exactly.

```cpp
auto line = pluss::make_series({-3.0, -1.0, 0.0, 2.0, 5.0}, {5});

auto composed = line.clamp_min(-1.0).clamp_max(2.0);
auto direct = line.clamp(-1.0, 2.0);

std::cout << "composed=" << composed << "\n";
std::cout << "direct=" << direct;
```

### Output

```text
composed=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -1, 0, 2, 2]
direct=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-1, -1, 0, 2, 2]
```

## Notes

- `clamp()` is the two-sided combination of [`clamp_min()`](clamp_min.md)
  and [`clamp_max()`](clamp_max.md) — reach for the one-sided methods
  directly when only one bound applies, and for `clamp()` when both do.
- Unlike `clamp_min()`/`clamp_max()`, there is currently no `Series`-bound
  (per-element) overload of `clamp()` itself; compose the two one-sided
  methods for that case.
- `min_v > max_v` throws rather than silently producing an empty or
  inverted range — validate bounds before calling if they come from
  untrusted input.
- Use `clamp_()` when storage mutation is the goal; use `clamp()` when the
  bound itself needs to participate in a differentiable computation.

## See Also

- [`Series::clamp_min`](clamp_min.md): the one-sided floor, with a
  `Series`-bound (broadcastable) overload `clamp()` doesn't have.
- [`Series::clamp_max`](clamp_max.md): the one-sided ceiling, with the same
  broadcastable overload.
- [`Series::minimum`](minimum.md) / [`Series::maximum`](maximum.md):
  the underlying elementwise comparisons `clamp_min`/`clamp_max` are built
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chunk.html">
    <span class="label">Previous</span>
    <span class="title">chunk</span>
  </a>
  <a class="next-link" href="clamp_max.html">
    <span class="label">Next</span>
    <span class="title">clamp_max</span>
  </a>
</div>
