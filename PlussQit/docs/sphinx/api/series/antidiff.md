<!-- pluss-hand-authored -->

# Series::antidiff

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::AntidiffAccessor Series::antidiff

SeriesAntidiffProxy Series::antidiff()
SeriesAntidiffProxy Series::antidiff(const Series& x)
```

## What It Does

`antidiff` is a namespace-like accessor for numerical antidifferentiation of
an already-sampled `Series`: composite-trapezoidal integration
(`trapezoid`) and its running form (`cumulative_trapezoid`), stochastic integrals of sampled values (`ito`, `strat`, `theta`) and quadratic variation and covariation. Like
`linearalg`, it does not compute a value by itself — it selects the method
family and keeps the owning `Series` as the values being integrated.

## Why This Has Its Own Accessor

The name is deliberate. `antidiff` is the reverse of `Series::diff()`
(discrete forward differencing, `x[i+1] - x[i]`): both operate on an
already-materialized sequence of numbers, with no reference to any
generating function. `cumsum(diff(x))` reconstructs `x` exactly (up to one
boundary value); `antidiff().cumulative_trapezoid(x)` is the same *family*
of operation — a cumulative sum built from consecutive differences — but not
the literal same formula, since it weights each term by the trapezoidal
rule (approximating an integral) rather than replaying the raw differences.

This is deliberately **not** a home for continuous integration of an
arbitrary callable — `pluss::subsp::math::integ::quadra`, `pluss::subsp::math::integ::aleatoria`, and
the `Analytica` closed-form registry (PlussMa) cover that. Those need to
re-invoke a function at points they choose themselves; a single
already-computed `Series` cannot provide that, only its own already-sampled
values. See [`trapezoid`](../series-antidiff/trapezoid.md) for the full
reasoning.

## Calling Styles

The dot-style form passes the x-coordinates to individual methods:

```cpp
auto area = y.antidiff.trapezoid(x);
```

The proxy style binds the x-coordinates first:

```cpp
auto area = y.antidiff(x).trapezoid();
```

Both calls integrate `y` over `x`. A third form needs no x-coordinates at
all — a uniform spacing `dx` (default `1.0`) is enough:

```cpp
auto area_default = y.antidiff.trapezoid();      // dx = 1.0
auto area_dx = y.antidiff.trapezoid(0.5);        // dx = 0.5
```

The zero-argument dot-style form (`y.antidiff.trapezoid()`) resolves based
on whether a peer was bound: with a bound peer (`y.antidiff(x).trapezoid()`)
it integrates against that `x`; without one, it falls back to `dx = 1.0`.

## Method Pages

- [`trapezoid`](../series-antidiff/trapezoid.md): total area under `y`
  over `x` (or a uniform spacing), reduced along one dimension
- [`cumulative_trapezoid`](../series-antidiff/cumulative_trapezoid.md): the
  running antiderivative, one value per sample point

Use the
[Series::AntidiffAccessor API Reference](../../series_antidiff_accessor_api_reference.md)
as the landing page for this family.

## Returns

`Series::AntidiffAccessor` or `SeriesAntidiffProxy`
    A lightweight method-selection object bound to the owning `Series`, and
    optionally to a peer `Series` (the x-coordinates).

    Accessing `antidiff` does not copy numeric data and does not mutate the
    owner.

## Example

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

std::cout << "call_style=" << y.antidiff().trapezoid(x) << "\n";
std::cout << "dot_style=" << y.antidiff.trapezoid(x) << "\n";
std::cout << "peer_bound=" << y.antidiff(x).trapezoid() << "\n";
```

### Output

```text
call_style=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
dot_style=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
peer_bound=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=22
```

All three spellings integrate the same `y` over the same `x` and agree
exactly.

## Gotchas And Failure Modes

- `antidiff` is an accessor, not a materialized result.
- The zero-argument dot-style method (`y.antidiff.trapezoid()`) silently
  uses `dx = 1.0` unless a peer was bound via `y.antidiff(x)` first — it
  never throws for "forgetting" x, since dx-only integration is a
  legitimate, separate calling form, not an error state.
- `trapezoid`/`cumulative_trapezoid` both require `x` (when given) to be
  broadcastable against `y` along the reduced dimension; a genuinely
  incompatible length throws.

## See Also

- [Series::AntidiffAccessor API Reference](../../series_antidiff_accessor_api_reference.md)
- [`Series::AntidiffAccessor::trapezoid`](../series-antidiff/trapezoid.md)
- [`Series::AntidiffAccessor::cumulative_trapezoid`](../series-antidiff/cumulative_trapezoid.md)
- [`Series::diff`](diff.md): the discrete forward difference `antidiff` reverses.
- [`Series::cumsum`](cumsum.md): the plain (unweighted) cumulative sum
  `diff()`'s output telescopes back through exactly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="angle.html">
    <span class="label">Previous</span>
    <span class="title">angle</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
