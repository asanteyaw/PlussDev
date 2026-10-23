<!-- pluss-hand-authored -->

# Series::CalcAccessor API Reference

[Back to Series API Reference](series_api_reference.md)

`Series::CalcAccessor` groups callable differentiation and anchored callable
integration. It does not introduce a second derivative tape. The derivative
methods delegate to the centralized higher-order reverse transform, while
`antidiff()` builds quadrature from ordinary differentiable `Series`
operations. The root entrypoint and option-binding syntax are described on
[`Series::calc`](api/series/calc.md).

## Method Index

- [`antidiff`](api/series-calc/antidiff.md) evaluates first or repeated
  anchored integrals at every receiver value.
- [`diff`](api/series-calc/diff.md) returns a derivative of any non-negative
  order, either as the full derivative tensor or the elementwise diagonal.
- [`grad`](api/series-calc/grad.md) returns the first derivative of a scalar
  objective with the input's shape.
- [`hess`](api/series-calc/hess.md) returns the second derivative of a scalar
  objective with two input-axis blocks.
- [`jac`](api/series-calc/jac.md) returns the first derivative of an arbitrary
  output with output axes followed by input axes.
- [`options`](api/series-calc/options.md) inspects the defaults bound by
  `x.calc(options)`.

`diff` and `antidiff` are also accessors with named methods for stochastic
calculus (see Stochastic Calculus below):

- [`diff.ito`](api/series-calc/diff_ito.md): Itô's lemma, product and quotient
  rules, jumps and time dependence.
- [`diff.strat`](api/series-calc/diff_strat.md): the Stratonovich differential and
  drift conversion.
- [`diff.marcus`](api/series-calc/diff_marcus.md): the Marcus differential and
  jump flow.
- [`diff.malliavin`](api/series-calc/diff_malliavin.md): the discrete Malliavin
  derivative and the Skorokhod integral.
- [`antidiff.ito`](api/series-calc/antidiff_ito.md): Itô and θ-integrals along a path.
- [`antidiff.strat`](api/series-calc/antidiff_strat.md): Stratonovich integrals along
  a path.
- [`antidiff.marcus`](api/series-calc/antidiff_marcus.md): the Marcus integral
  along a path that jumps.

## Choosing A Derivative Form

Let the receiver have shape `X` and let `fn(x)` have shape `Y`.

| Call | Required callback output | Result shape |
| --- | --- | --- |
| `x.calc.diff(fn, n)` | Any differentiable `Y` | `Y + X` repeated `n` times |
| `x.calc({.elementwise=true}).diff(fn, n)` | Exactly `X` | `X` |
| `x.calc.grad(fn)` | Rank-zero scalar | `X` |
| `x.calc.jac(fn)` | Any differentiable `Y` | `Y + X` |
| `x.calc.hess(fn)` | Rank-zero scalar | `X + X` |

`grad`, `jac`, and `hess` are constrained call forms over the same engine as
`diff`; they are not separate differentiation systems. Their names make the
expected geometry explicit and reject incompatible callback outputs early.

## Graph And Dependency Contract

The callback must build its result from differentiable `Series` operations.
The transform creates a fresh differentiable leaf internally, evaluates the
callback, runs reverse mode with graph retention when another derivative order
is needed, and returns the requested derivative. The original receiver need
not call `tracks_slope_()` and does not receive a stored sensitivity.

A derivative is only as differentiable as every operation in the callback's
reverse path. Higher-order requests fail when an operation declares that its
reverse rule cannot itself be differentiated. With `strict_dependency=false`,
a genuinely disconnected output contributes zero; with
`strict_dependency=true`, disconnection is an error. This distinction prevents
an unsupported higher-order path from being silently advertised as a true
zero.

## Callable Integration

For a receiver `u`, `u.calc.antidiff(fn, a)` returns one value per endpoint:

```text
F(u) = integral from a to u of fn(t) dt
```

The receiver may be scalar or higher-rank. Quadrature nodes are appended as a
temporary last axis, so the callback must be vectorized and return exactly the
node shape it receives. Repeated integration uses the equivalent weighted
single-integral formula and optional lower-bound derivatives; it does not nest
host loops or repeatedly invoke a scalar integrator.

Because endpoint construction and quadrature are composed from differentiable
`Series` primitives, reverse mode can continue through the returned integral.

## Stochastic Calculus

The same two accessors carry the calculus of random processes. Three ideas
organize it.

- A **differential** (`x.calc.diff.ito(g, mu, sigma)`) takes a function `g` and the
  coefficients of a process `dX = μ dt + σ dW` and returns the coefficients of
  `g(X)`: the drift and diffusion, and for processes that jump the effect of each
  jump. Every element of the receiver is an independent point, so one call covers a
  grid or a batch of independent processes.
- An **integral** (`w.calc.antidiff.ito(f, t0, t1)`) integrates a function of time
  and path against a sampled path. The receiver is the path.
- The **convention** decides what the symbols mean. Itô reads the integrand at the
  start of each step; Stratonovich reads the average of both ends and obeys the
  ordinary chain rule; Marcus extends the Stratonovich rule to paths that jump.

| Task | Call |
| --- | --- |
| Coefficients of `g(X)`, Itô | [`x.calc.diff.ito(g, mu, sigma)`](api/series-calc/diff_ito.md) |
| Coefficients of `g(X)`, ordinary chain rule | [`x.calc.diff.strat(g, mu, sigma)`](api/series-calc/diff_strat.md) |
| Change the convention of a drift | `x.calc.diff.strat_drift` / `ito_drift` |
| Jumps that follow a flow | [`x.calc.diff.marcus(...)`](api/series-calc/diff_marcus.md) |
| Derivative with respect to Brownian increments | [`dw.calc.diff.malliavin(F)`](api/series-calc/diff_malliavin.md) |
| Integral of `f(t, W)` against a path, Itô | [`w.calc.antidiff.ito(f, t0, t1)`](api/series-calc/antidiff_ito.md) |
| The same, Stratonovich | [`w.calc.antidiff.strat(f, t0, t1)`](api/series-calc/antidiff_strat.md) |
| The same, for an integrand already sampled | [`f.antidiff(w).ito()`](api/series-antidiff/stochastic_integrals.md) |
| Quadratic variation and covariation | [`w.antidiff().quadratic_variation()`](api/series-antidiff/quadratic_variation.md) |

Simulating stochastic differential equations, checking that an integrand is adapted
and diagnosing a path that is not a standard Wiener path belong to the numerical
methods library; it forwards to the methods on these pages.

## Bound Options

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.order = 2;
options.lower = 0.0;
options.initial = {2.0, 3.0};

auto configured = x.calc(options);
auto derivative = configured.diff(fn);       // order 2
auto integral = configured.antidiff(fn);     // lower 0, order 2
```

An explicit terminal argument wins over the corresponding bound field:
`configured.diff(fn, 1)` uses order one, and
`configured.antidiff(fn, -1.0)` uses lower bound `-1.0`. See
[`options`](api/series-calc/options.md) for every field and its scope.

```{toctree}
:hidden:
:maxdepth: 1

api/series-calc/antidiff
api/series-calc/antidiff_ito
api/series-calc/antidiff_strat
api/series-calc/antidiff_marcus
api/series-calc/diff
api/series-calc/diff_ito
api/series-calc/diff_strat
api/series-calc/diff_marcus
api/series-calc/diff_malliavin
api/series-calc/grad
api/series-calc/hess
api/series-calc/jac
api/series-calc/options
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/series/calc.html"><span class="label">Previous</span><span class="title">calc</span></a>
  <a class="next-link" href="api/series-calc/antidiff.html"><span class="label">Next</span><span class="title">antidiff</span></a>
</div>
