<!-- generated from GLMOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::GLMOptions`

Everything `glm_fit` can be told besides the data: the family and link, the confidence level, how the intercept and missing rows are treated, the iteration limits, the case weights and offset, and the labels.

## Signature

```cpp
regression::GLMOptions options;
regression::GLMFitResult regression::glm_fit(const Series &y, const Series &x, const regression::GLMOptions &options = {}, Config cfg = {});
```

A default `GLMOptions` fits a Normal model with the identity link and an intercept at the 95% level. Set only the members you need: `reg::GLMOptions options; options.family = reg::GLMFamily::Poisson;`.

## Fields

### `family`

`regression::GLMFamily`, default `Normal`.

The distribution of the response. See [`GLMFamily`](GLMFamily.md); the choice fixes the variance function, the canonical link, which responses are legal and whether the dispersion is estimated. The default is `Normal`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  out.writeln("default family is Normal:", options.family == reg::GLMFamily::Normal);
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("coefficients", fit.coefficients.items<double>(), " dispersion", fit.dispersion);
}
```

Output

```text
default family is Normal: true
coefficients [-0.54808, 0.63117]  dispersion 1.00000
```

### `link`

`regression::GLMLink`, default `Canonical`.

The link between the mean and the linear predictor. See [`GLMLink`](GLMLink.md). The default `Canonical` resolves to the family's own link; naming another changes the model, not just its scale. The program fits the same counts with the canonical log link and with the identity link, and reports whether each converged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto canonical = reg::glm_fit(y, x, options);
  options.link = reg::GLMLink::Identity;
  const auto identity = reg::glm_fit(y, x, options);
  out.writeln("canonical (log) coefficients", canonical.coefficients.items<double>(), " converged", canonical.converged, " iterations", canonical.iterations);
  out.writeln("identity coefficients       ", identity.coefficients.items<double>(), " converged", identity.converged, " iterations", identity.iterations);
}
```

Output

```text
canonical (log) coefficients [-0.54808, 0.63117]  converged true  iterations 6
identity coefficients        [0.15604, 1.68792]  converged false  iterations 200
```

### `alpha`

`double`, default `0.05`.

The significance level of the confidence intervals, so the intervals have coverage 1 − alpha. The default 0.05 gives 95% intervals; a larger alpha gives narrower intervals. It changes only `conf_int`: the coefficients, standard errors and p-values do not depend on it. The program fits the counts at 0.05 and 0.10 and prints the interval of the slope.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  for (const double alpha : {0.05, 0.10}) {
    options.alpha = alpha;
    const auto fit = reg::glm_fit(y, x, options);
    const auto ci = fit.conf_int.reshape({4}).items<double>();   // [lower0, upper0, lower1, upper1]
    out.writeln("alpha", alpha, " coverage", 1.0 - alpha, " slope", fit.coefficients.items<double>()[1], " interval", ci[2], ci[3]);
  }
}
```

Output

```text
alpha 0.05000  coverage 0.95000  slope 0.63117  interval 0.38564 0.87670
alpha 0.10000  coverage 0.90000  slope 0.63117  interval 0.42511 0.83722
```

### `add_intercept`

`bool`, default `true`.

Whether a constant column is added to the design. With the default `true` the first coefficient is the intercept and is named `const`; with `false` the model has only the supplied predictors, so the linear predictor is exactly zero when every predictor is zero. Dropping an intercept that the data need biases every other coefficient; the program shows the slope changing from 0.63 to 0.50 when it is removed, and the names and the model degrees of freedom of each fit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto with = reg::glm_fit(y, x, options);
  options.add_intercept = false;
  const auto without = reg::glm_fit(y, x, options);
  out.writeln("with intercept   ", with.coefficient_names, with.coefficients.items<double>(), " df_model", with.df_model, " has_intercept", with.has_intercept);
  out.writeln("without intercept", without.coefficient_names, without.coefficients.items<double>(), " df_model", without.df_model, " has_intercept", without.has_intercept);
}
```

Output

```text
with intercept    [const, x1] [-0.54808, 0.63117]  df_model 1  has_intercept true
without intercept [x1] [0.49951]  df_model 1  has_intercept false
```

### `drop_missing`

`bool`, default `true`.

What to do with rows that hold NaN or infinite values. With the default `true`, such rows are left out and counted in `dropped_observations`. With `false`, any non-finite value is an error. The program fits with one missing predictor, then asks for the strict behaviour.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  auto values = std::vector<double>{0.5, 1.0, std::nan(""), 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0};
  const auto holey = pluss::make_series(values, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fit = reg::glm_fit(y, holey, options);
  out.writeln("drop_missing=true : nobs", fit.nobs, " dropped", fit.dropped_observations);
  options.drop_missing = false;
  try {
    reg::glm_fit(y, holey, options);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln("drop_missing=false:", text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
drop_missing=true : nobs 9  dropped 1
drop_missing=false: subsp::stats::regression::glm_fit: non-finite values found and drop_missing=false
```

### `rcond`

`double`, default `1e-12`.

The reciprocal-condition cutoff below which the weighted normal equations treat a direction of the design as numerically absent. Raising it declares nearly collinear predictors redundant sooner; the result's `rank` falls and the fit is returned for the identified part, with the effect shared between the redundant columns. The program fits a design whose second column is the first plus a tiny perturbation, once at the default and once at a coarse cutoff.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  std::vector<double> flat;
  for (int i = 0; i < 10; ++i) flat.push_back(0.5 * (i + 1));
  for (int i = 0; i < 10; ++i) flat.push_back(0.5 * (i + 1) + 1e-5 * ((i % 3) - 1));
  const auto x2 = pluss::make_series(flat, {2, 10}, as_f64()).transpose(0, 1);
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto fine = reg::glm_fit(y, x2, options);
  options.rcond = 1e-3;
  const auto coarse = reg::glm_fit(y, x2, options);
  out.writeln("rcond 1e-12: rank", fine.rank, " of", fine.k_params, " condition number", fine.condition_number);
  out.writeln("rcond 1e-3 : rank", coarse.rank, " of", coarse.k_params, " coefficients", coarse.coefficients.items<double>());
}
```

Output

```text
rcond 1e-12: rank 3  of 3  condition number 763738.86838
rcond 1e-3 : rank 2  of 3  coefficients [-0.54808, 0.31559, 0.31558]
```

### `max_iter`

`std::int64_t`, default `200`.

The cap on iteratively reweighted least-squares steps. If it is reached before the largest change in a coefficient falls to `tol` times one plus the largest coefficient, the fit is returned anyway with `converged` false and a warning. The program allows two iterations and then the default.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.max_iter = 2;
  const auto short_run = reg::glm_fit(y, x, options);
  options.max_iter = 200;
  const auto full = reg::glm_fit(y, x, options);
  out.writeln("max_iter 2  : converged", short_run.converged, " iterations", short_run.iterations, " coefficients", short_run.coefficients.items<double>());
  out.writeln("warning     :", short_run.warnings);
  out.writeln("max_iter 200: converged", full.converged, " iterations", full.iterations, " coefficients", full.coefficients.items<double>());
}
```

Output

```text
max_iter 2  : converged false  iterations 2  coefficients [-0.41216, 0.60653]
warning     : [IRLS reached max_iter without satisfying tolerance.]
max_iter 200: converged true  iterations 6  coefficients [-0.54808, 0.63117]
```

### `tol`

`double`, default `1e-8`.

The convergence tolerance: the iteration stops when the largest change in a coefficient is at most `tol` times one plus the largest coefficient. A looser tolerance stops earlier and a tighter one iterates longer. Here four iterations at 1e-2 already agree with the six at the default 1e-8 to five decimals, so a loose tolerance is enough when only a few digits matter. The program fits at 1e-2 and at the default 1e-8.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.tol = 1e-2;
  const auto loose = reg::glm_fit(y, x, options);
  options.tol = 1e-8;
  const auto tight = reg::glm_fit(y, x, options);
  out.writeln("tol 1e-2: iterations", loose.iterations, " coefficients", loose.coefficients.items<double>());
  out.writeln("tol 1e-8: iterations", tight.iterations, " coefficients", tight.coefficients.items<double>());
}
```

Output

```text
tol 1e-2: iterations 4  coefficients [-0.54807, 0.63117]
tol 1e-8: iterations 6  coefficients [-0.54808, 0.63117]
```

### `weights`

`Series`, default `undefined`.

Case weights, one non-negative value per row, that multiply each observation's contribution to the likelihood. A weight of two counts the row twice; a weight of zero removes the row, and it is counted in `dropped_observations`. The program gives the last row weight zero and compares with the fit that leaves that row out by hand.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {10}, as_f64());
  const auto weighted = reg::glm_fit(y, x, options);
  options.weights = pluss::series::undefined();
  const auto by_hand = reg::glm_fit(y.slice(0, 0, 9), x.slice(0, 0, 9), options);
  out.writeln("weight zero: nobs", weighted.nobs, " dropped", weighted.dropped_observations, " coefficients", weighted.coefficients.items<double>());
  out.writeln("row removed: nobs", by_hand.nobs, "         coefficients", by_hand.coefficients.items<double>());
}
```

Output

```text
weight zero: nobs 9  dropped 1  coefficients [-0.50228, 0.61410]
row removed: nobs 9          coefficients [-0.50228, 0.61410]
```

### `offset`

`Series`, default `undefined`.

A known term added to the linear predictor, one value per row, whose coefficient is fixed at one. The classic use is exposure in a rate model: a Poisson count over different exposures has log-mean ln(exposure) + Xβ, so the offset is the log of the exposure and the coefficients are effects on the rate. The program fits counts observed over different exposure times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto exposure = pluss::make_series(std::vector<double>{1, 1, 2, 2, 2, 3, 3, 3, 4, 4}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.offset = exposure.log();
  const auto rate = reg::glm_fit(y, x, options);
  options.offset = pluss::series::undefined();
  const auto plain = reg::glm_fit(y, x, options);
  out.writeln("with log(exposure) offset", rate.coefficients.items<double>());
  out.writeln("ignoring the exposure    ", plain.coefficients.items<double>());
}
```

Output

```text
with log(exposure) offset [-0.68086, 0.36902]
ignoring the exposure     [-0.54808, 0.63117]
```

### `response_name`

`std::string`, default `"y"`.

The label of the response, carried to the result and printed by the summary. It does not affect the fit. The default is `y`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}, {10}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  options.response_name = "events";
  const auto fit = reg::glm_fit(y, x, options);
  out.writeln("response_name", fit.response_name);
  out.writeln(reg::glm_summary(fit));
}
```

Output

```text
response_name events
                            GLM Regression Results
==============================================================================
Dep. Variable:                    events   Family:                          poisson
Model:                               GLM   Link:                                log
Method:                             IRLS   Pseudo R-squared:                 0.8912
Rank:                                  2   No. Iterations:                        6
Alpha:                              0.05   Converged:                          true
No. Observations:                     10   Df Residuals:                          8
Df Model:                              1   Log-Likelihood:                   -16.65
Deviance:                          3.857   AIC:                                37.3
Null Deviance:                     35.45   BIC:                               37.91
==============================================================================
                        coef     std err           z       P>|z|      [0.025      0.975]
------------------------------------------------------------------------------
const                -0.5481      0.5064     -1.0822      0.2792     -1.5407      0.4445
x1                    0.6312      0.1253      5.0383  4.6963e-07      0.3856      0.8767
==============================================================================
Dispersion:                            1   Cond. No.                          7.261
==============================================================================
```

### `predictor_names`

`std::vector<std::string>`, default `empty`.

Labels for the predictors, one per column of `x`, used for `coefficient_names` and in the summary. The intercept is always named `const`. Without names the predictors are called `x1`, `x2`, and so on. The program names a two-predictor model.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto y = pluss::make_series(std::vector<double>{1, 0, 2, 3, 2, 5, 4, 8, 9, 14}, {10}, as_f64());
  std::vector<double> flat;
  for (int i = 0; i < 10; ++i) flat.push_back(0.5 * (i + 1));
  for (int i = 0; i < 10; ++i) flat.push_back((i % 4) * 0.7);
  const auto x2 = pluss::make_series(flat, {2, 10}, as_f64()).transpose(0, 1);
  reg::GLMOptions options;
  options.family = reg::GLMFamily::Poisson;
  const auto unnamed = reg::glm_fit(y, x2, options);
  options.predictor_names = {"dose", "batch"};
  const auto named = reg::glm_fit(y, x2, options);
  out.writeln("default names", unnamed.coefficient_names);
  out.writeln("given names  ", named.coefficient_names);
}
```

Output

```text
default names [const, x1, x2]
given names   [const, dose, batch]
```

## See Also

[`glm_fit`](glm_fit.md) consumes the options, [`GLMFamily`](GLMFamily.md) and [`GLMLink`](GLMLink.md) define the first two, and [`GLMFitResult`](GLMFitResult.md) records the resolved values and the outcome.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="GLMPredictionType.html">
    <span class="label">Previous</span>
    <span class="title">GLMPredictionType</span>
  </a>
  <a class="next-link" href="GLMFitResult.html">
    <span class="label">Next</span>
    <span class="title">GLMFitResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
