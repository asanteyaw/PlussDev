<!-- generated from plussma_regression.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Regression API Reference

Regression is available through the public namespace `pluss::ma::regression`. The compact `pluss::ma::reg` alias names the same API, but this reference uses the full spelling so the statistical concern remains visible at call sites.

```cpp
#include <pluss/ma.h>

namespace regression = pluss::ma::regression;
```

The API consumes `Series` responses and predictor designs. A one-dimensional predictor is one feature; a two-dimensional predictor is an `n × p` design whose rows are observations and whose columns are predictors. Fitting returns a structured result rather than hiding coefficients, uncertainty, residuals and diagnostics behind mutable model state.

## Families

| Family | What it covers |
|---|---|
| [Least squares](regression/least_squares.md) | Ordinary and case-weighted linear regression, prediction and statistical summaries |
| [Generalised linear models](regression/glm.md) | Normal, binomial and Poisson regression with offsets, weights, prediction and reports |
| [Quantile regression](regression/quantile.md) | Conditional quantiles by the check loss, with robust or classical standard errors |
| [Polynomial regression](regression/polynomial.md) | Powers and cross terms of the predictors fitted by least squares |

The {guide}`regression guide <regress>` covers worked problems and the statistics behind each family in longer form.

## Common Shape Rules

- `y` is one-dimensional with one response per observation.
- `x` is either `[n]` for one predictor or `[n, p]` for `p` predictors.
- Prediction accepts the same predictor width used at fit time. A single multi-feature observation must therefore be `[1, p]`, not `[p]`.
- Case weights are one-dimensional, finite and non-negative. A zero-weight row is excluded from estimation and counted as dropped.
- Numerical work is vectorized through the `Series` backend. Configuration can select dtype and platform without changing the regression contract.

```{toctree}
:maxdepth: 3
:hidden:

regression/least_squares
regression/glm
regression/quantile
regression/polynomial
```
