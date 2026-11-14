# User Guide

Task-oriented guides to PlussMa, one per topic. Each guide is a short series of complete programs whose printed output is the real output of the program, with the reasoning around them. Every function, type and option the guides use has its own page in the [API Reference](../api_reference.md), and the guides link to it.

| Topic | What it covers | Reference |
|---|---|---|
| [Time series](ts/guide.md) | Transforms, diagnostics, ARIMA, volatility, vector, state-space and direct forecasting models. | [API reference](../api/plussma_ts.md) |
| [Distributions](distr.md) | Sixty probability families, sampling and maximum-likelihood fitting. | [API reference](../api/plussma_distr.md) |
| [Hypothesis testing](infer.md) | Tests of means, variances, ranks, distributions and tables, with exact small-sample p-values. | [API reference](../api/plussma_infer.md) |
| [Regression](regress.md) | Least squares, generalised linear, quantile and polynomial regression. | [API reference](../api/plussma_regression.md) |
| [Optimization](optim.md) | Constrained and derivative-free optimization, linear and mixed-integer programming, root finding. | [API reference](../api/plussma_optim.md) |
| [Calculus](calculus.md) | Functions as values: exact derivatives, integrals and vector calculus, and operators that transform them. | [API reference](../api/plussma_calculus.md) |
| [Numerical methods](numerics.md) | Integration, differential equations, interpolation and large linear systems. | [API reference](../api/plussma_numerics.md) |
| [Stochastic processes](stochastic.md) | Stochastic integrals, Itô calculus, jumps, SDE simulation and Malliavin calculus. | [API reference](../api/plussma_stochastic.md) |
| [Finance](finance.md) | Option pricing: Black-Scholes, implied volatility and stochastic-volatility models. | [API reference](../api/plussma_finance.md) |
| [Actuarial mathematics](actuarial.md) | Mortality and life tables, contingent present values, aggregate losses, ratemaking and reserving. | [API reference](../api/plussma_actuarial.md) |
| [Engineering](eng.md) | Signal processing, dynamic systems, controls, circuits, power, fluids and structures. | [API reference](../api/plussma_eng.md) |

```{toctree}
:maxdepth: 3
:hidden:

ts/guide
distr
infer
regress
optim
calculus
numerics
stochastic
finance
actuarial
eng
```
