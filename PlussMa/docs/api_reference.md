# API Reference

PlussMa is organized by public framework first, so you can enter through the exact namespace you are using in code.

| Framework | What it covers | Reference |
|---|---|---|
| `ts` | Time series: transforms, diagnostics, ARIMA, volatility, vector, state-space and direct forecasting models | [API reference](api/plussma_ts.md) |
| Distributions | Probability families, sampling and maximum-likelihood fitting | [API reference](api/plussma_distr.md) · [Guides](manual/distr.md) |
| Inference | Tests of means, variances, ranks, distributions and tables | [API reference](api/plussma_infer.md) · [Guides](manual/infer.md) |
| Regression | Least squares, generalised linear, quantile and polynomial regression | [Guides](manual/regress.md) |
| Optimization | Constrained and unconstrained optimization, linear and mixed-integer programming, derivative-free search, root finding | [Guides](manual/optim.md) |
| Numerical methods | Integration, differential equations, interpolation and large linear systems | [Guides](manual/numerics.md) |
| Stochastic processes | Stochastic integrals, Itô calculus, jump processes, SDE simulation | [Guides](manual/stochastic.md) |

Time series, distributions and inference have a full API reference, one page per function or family. The other frameworks are documented through their guides, which cover every public entry point with complete programs and real output.

```{toctree}
:maxdepth: 4
:hidden:

api/plussma_ts
api/plussma_distr
api/plussma_infer
```
