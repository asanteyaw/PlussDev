<!-- pluss-hand-authored -->

# SeriesAntidiffAccessor::trapezoid

[Back to SeriesAntidiffAccessor API Reference](../../series_antidiff_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesAntidiffAccessor::trapezoid(const Series& x, std::int64_t dim = -1) const
Series SeriesAntidiffAccessor::trapezoid(double dx, std::int64_t dim = -1) const
Series SeriesAntidiffAccessor::trapezoid(std::int64_t dim = -1) const

Series SeriesAntidiffProxy::trapezoid(const Series& x, std::int64_t dim = -1) const
Series SeriesAntidiffProxy::trapezoid(double dx, std::int64_t dim = -1) const
Series SeriesAntidiffProxy::trapezoid(std::int64_t dim = -1) const

Series pluss::integ::trapezoid(const Series& y, const Series& x, std::int64_t dim = -1)
Series pluss::integ::trapezoid(const Series& y, double dx, std::int64_t dim = -1)
Series pluss::integ::trapezoid(const Series& y, std::int64_t dim = -1)
```

## What It Does

`trapezoid()` approximates the definite integral of already-sampled data
using the composite trapezoidal rule: each pair of adjacent samples is
treated as the parallel sides of a trapezoid, and the areas of every
trapezoid along `dim` are summed.

For two adjacent samples `y[i]`, `y[i+1]` separated by `x[i+1] - x[i]`, the
trapezoid's area is

```text
0.5 * (y[i] + y[i+1]) * (x[i+1] - x[i])
```

Summing this over every adjacent pair gives the total. This is the same
algorithm `torch.trapezoid` and `numpy.trapz` implement — real torch's own
`native_functions.yaml` declares `trapezoid` with no `dispatch:` entry,
meaning torch itself computes it compositionally from other primitives
rather than a dedicated kernel; `Series::trapezoid` does the same, built
from `diff`, `slice`, `mul`, `add`, and `sum`.

## Why "antidiff", Not "integ"

`trapezoid()` lives on the `antidiff` accessor because it reverses
`Series::diff()` — both operate on an already-materialized sequence of
numbers, never on a callable. This is a genuinely different object from
`pluss::integ::quadra()`/`aleatoria()` (PlussMa), which integrate an
arbitrary *function* by evaluating it at points they choose adaptively. A
`Series` here only ever provides the values it already holds; `trapezoid()`
cannot ask for a value at a point that was not sampled, which is exactly
what distinguishes sampled-data quadrature from function quadrature. The
`pluss::integ::trapezoid` free functions exist purely as a namespaced
convenience matching this module's other calling conventions
(`pluss::integ::quadra`, `pluss::integ::aleatoria`) — they forward to this
exact accessor and share its implementation; there is no duplicate formula
to keep in sync.

## Three Ways To Supply The Domain

- **Explicit x-coordinates** (`trapezoid(x)`): use this when samples are
  unevenly spaced, or the coordinate axis is meaningful data in its own
  right (time stamps, strike prices, ...).
- **Uniform spacing** (`trapezoid(dx)`): use this when samples are
  evenly spaced and only the spacing matters, not the absolute coordinate
  values.
- **Default spacing** (`trapezoid()`, `dx = 1.0`): the zero-argument
  dot-style form. When called through a peer-bound proxy
  (`y.antidiff(x).trapezoid()`), it uses the bound `x` instead of
  defaulting — see the accessor's own binding rule in
  [`Series::antidiff`](../series/antidiff.md).

## Parameters

**x** (`const Series&`)
    The coordinate values corresponding to each sample of `y` along `dim`.
    Must be broadcastable against `y` along that dimension. May be complex,
    matching `y`'s own dtype, for a complex-valued domain.

**dx** (`double`)
    A uniform spacing between samples, used in place of explicit
    coordinates.

**dim** (`std::int64_t`, default `-1`)
    The dimension to integrate over. Any other, leading dimensions are
    treated as independent batch rows (see Batching below).

## Returns

`Series`
    A new `Series` with `dim` reduced away (removed, not kept as a
    length-one axis). For a 1-D `y`, the result is a rank-zero scalar. For
    batched `y`, the result has one entry per batch row.

## Batching

Every reduction happens along `dim` specifically, never a hardcoded axis,
so `y` may carry any number of leading batch dimensions sharing the same
`x`. One call prices a whole batch — a strike/maturity grid, or many
independent paths — in one vectorized pass. See Example 4.

## Gradient

`trapezoid()` has no custom backward rule anywhere — it inherits its
gradient entirely from the composed `diff`/`slice`/`mul`/`add`/`sum` ops'
own already-verified reverse-mode rules. For uniform `dx`, the gradient has
a clean closed form: `d(trapezoid)/dy[i]` is `0.5 * dx` at a boundary
sample and `dx` at every interior sample — each sample's trapezoid weight.
See Example 5.

## Examples

Example 1. The two binary call styles are equivalent.

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

std::cout << "call_style=" << y.antidiff().trapezoid(x) << "\n";
std::cout << "dot_style=" << y.antidiff.trapezoid(x) << "\n";
```

### Output

```text
call_style=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
dot_style=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
```

Example 2. Explicit x-coordinates, a uniform `dx`, and the zero-argument
default all agree when the spacing matches.

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
auto x = pluss::make_series({0.0, 0.5, 1.0, 1.5, 2.0});

std::cout << "with_x=" << y.antidiff.trapezoid(x) << "\n";
std::cout << "with_dx=" << y.antidiff.trapezoid(0.5) << "\n";
std::cout << "no_arg_default=" << y.antidiff.trapezoid() << "\n";
```

`with_x` and `with_dx` both use spacing `0.5` and agree. `no_arg_default`
uses the fallback `dx = 1.0` instead, so it is not the same integral — it is
double `with_dx`'s value, matching the doubled spacing.

### Output

```text
with_x=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=11
with_dx=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=11
no_arg_default=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
```

Example 3. Binding a peer via the proxy style, then calling with no
arguments.

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

std::cout << "peer_bound=" << y.antidiff(x).trapezoid() << "\n";
```

### Output

```text
peer_bound=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
```

Example 4. Batching: one call prices two independent rows sharing the same
`x`.

```cpp
auto y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});

std::cout << y.antidiff.trapezoid(x, -1) << "\n";
```

The second row is exactly ten times the first, and its integral scales the
same way.

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7.5, 75]
```

Example 5. The gradient is the trapezoid weight of each sample: `0.5` at
each boundary, `1.0` at every interior point (uniform `dx = 1.0`).

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});
y.tracks_slope_(true);

y.antidiff.trapezoid(x).reverse();

std::cout << "dArea_dy=" << y.sensitivity() << "\n";
```

### Output

```text
dArea_dy=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.5, 1, 1, 1, 0.5]
```

Example 6. Complex `y` works with no special handling — the formula is a
pure linear combination, so real and imaginary channels are independent.

```cpp
auto y = pluss::make_series(std::vector<std::complex<double>>{{0.0, 0.0}, {1.0, 2.0}, {4.0, 8.0}}, {3})
              .to(pluss::ScalarType::Complex128);
auto x = pluss::make_series({0.0, 1.0, 2.0}).to(pluss::ScalarType::Complex128);

std::cout << "complex_area=" << y.antidiff.trapezoid(x) << "\n";
```

### Output

```text
complex_area=pluss::Series(shape=[], strides=[], dtype=complex128, platform=cpu, numel=1, contiguous=true)
data=(3+6j)
```

Example 7. A genuinely incompatible `x` length is rejected, not silently
broadcast or truncated.

```cpp
try {
  auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
  auto x = pluss::make_series({0.0, 1.0, 2.0});
  std::cout << y.antidiff.trapezoid(x) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
broadcast: incompatible shapes
```

## Gotchas And Failure Modes

- The zero-argument dot-style form (`y.antidiff.trapezoid()`) uses
  `dx = 1.0` unless a peer was bound first — it is a legitimate calling
  form, not a missing-argument error.
- `x`'s length along `dim` must be broadcastable against `y`'s; a genuine
  mismatch throws rather than silently truncating or padding.
- Descending `x` (e.g. `x = [4, 3, 2, 1, 0]`) is not special-cased —
  `diff(x)` is naturally negative, and the formula produces the correctly
  signed (negative) result the same way torch's own does.
- `trapezoid()` is out-of-place; it never mutates `y` or `x`.

## Summary

`trapezoid()` is composite-trapezoidal quadrature over already-sampled
data — the total area, reduced along one dimension, batchable, and
differentiable for free. Reach for `pluss::integ::quadra`/`aleatoria`
instead when the integrand is a function you can call at arbitrary points,
not data you already have in hand.

## See Also

- [`SeriesAntidiffAccessor::cumulative_trapezoid`](cumulative_trapezoid.md):
  the running form of this same computation.
- [`Series::diff`](../series/diff.md): the discrete forward difference this
  accessor reverses.
- [`Series::cumsum`](../series/cumsum.md): the underlying reduction
  `cumulative_trapezoid()` builds on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../series_antidiff_accessor_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">SeriesAntidiffAccessor</span>
  </a>
  <a class="next-link" href="cumulative_trapezoid.html">
    <span class="label">Next</span>
    <span class="title">cumulative_trapezoid</span>
  </a>
</div>
