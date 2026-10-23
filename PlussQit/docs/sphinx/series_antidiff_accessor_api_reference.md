<!-- pluss-hand-authored -->

# Series::AntidiffAccessor API Reference

[Back to Series API Reference](series_api_reference.md)

This page is the landing page for the antiderivative workflow entered through
[`Series::antidiff`](api/series/antidiff.md) and continued through
`Series::AntidiffAccessor`. The accessor groups sampled-data antidifferentiation
methods — composite-trapezoidal integration and its running form — under one
namespace, the same way `linearalg` groups vector/matrix methods and `stat`
groups descriptive statistics.

## Using This Reference

- Use the method rail to open one accessor method at a time.
- Each method page owns its signatures, shape contract, gradient model,
  verified examples, failures, and previous/next navigation.
- Start at [`Series::antidiff`](api/series/antidiff.md) when you need the
  accessor calling model; start here when you already know which operation
  you need.

## Read This First

The accessor supports three equivalent-in-spirit calling styles:

```cpp
auto area1 = y.antidiff.trapezoid(x);      // dot-style, x passed directly
auto area2 = y.antidiff(x).trapezoid();    // proxy style, x bound first
auto area3 = y.antidiff.trapezoid(0.5);    // dx-only, no x Series at all
```

The first two integrate `y` against explicit x-coordinates and always agree.
The third needs no x-coordinates — a uniform spacing `dx` is enough, and it
defaults to `1.0` when omitted entirely (`y.antidiff.trapezoid()`).

`antidiff` is named for what it reverses: `Series::diff()`, the discrete
forward difference. Both operate on an already-materialized sequence of
numbers. It is deliberately not the accessor for integrating an arbitrary
*function* — that is `pluss::integ::quadra`/`aleatoria`/`Analytica`
(PlussMa), which need to re-invoke a callable at points they choose, not
read off values a `Series` already holds. See
[`Series::antidiff`](api/series/antidiff.md) for the full distinction.

## Method Pages

- [`trapezoid`](api/series-antidiff/trapezoid.md): total area under `y` over
  `x` (or a uniform spacing), reduced along one dimension — torch parity
  with `torch.trapezoid`
- [`cumulative_trapezoid`](api/series-antidiff/cumulative_trapezoid.md): the
  running antiderivative, one value per sample point — torch parity with
  `torch.cumulative_trapezoid`

- [`ito`, `strat`, `theta`](api/series-antidiff/stochastic_integrals.md): stochastic
  integrals of an integrand that is already sampled, against a sampled path
- [`quadratic_variation`, `covariation`](api/series-antidiff/quadratic_variation.md):
  sums of squared and cross increments of sampled paths

The handwritten reference covers the complete `Series::AntidiffAccessor`
surface.

## Choosing Between Them

Use `trapezoid()` when only the total area is needed — a single number (or
one number per batch row, see below). Use `cumulative_trapezoid()` when the
running antiderivative itself is needed at every sample point, not just its
final value; `cumulative_trapezoid()`'s last entry always equals
`trapezoid()`'s result computed over the same data, since the total area is
the running sum's final term.

## Batching

Both methods reduce (or accumulate) along one dimension only (`dim`,
default the last axis). Any number of leading axes are treated as
independent batch rows sharing the same x-coordinates, so one call prices
an entire batch — a whole strike/maturity grid, or many independent
Monte Carlo paths — without a per-row loop.

## Example

```cpp
auto y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});

std::cout << y.antidiff.trapezoid(x, -1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7.5, 75]
```

Two independent rows, one shared `x`, one call.

## Gotchas And Failure Modes

- `antidiff` is an accessor, not a materialized result.
- The zero-argument dot-style form silently falls back to `dx = 1.0` unless
  a peer was bound via `y.antidiff(x)` first — this is a legitimate calling
  form, not a missing-argument error.
- `x` (when supplied) must be broadcastable against `y` along the reduced
  dimension; a genuinely incompatible length throws.
- Both methods are built entirely from ordinary differentiable `Series`
  primitives (`diff`, `slice`, `mul`, `add`, `sum`/`cumsum`), so gradients
  flow through them for free — no custom backward rule exists for either.

```{toctree}
:hidden:
:maxdepth: 1

api/series-antidiff/trapezoid
api/series-antidiff/cumulative_trapezoid
api/series-antidiff/stochastic_integrals
api/series-antidiff/quadratic_variation
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/series/antidiff.html">
    <span class="label">Previous</span>
    <span class="title">antidiff</span>
  </a>
  <a class="next-link" href="api/series-antidiff/trapezoid.html">
    <span class="label">Next</span>
    <span class="title">trapezoid</span>
  </a>
</div>
