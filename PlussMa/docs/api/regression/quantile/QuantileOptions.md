<!-- generated from QuantileOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `regression::QuantileOptions`

Everything `quantile_fit` can be told besides the data: the quantile, the covariance estimator, the confidence level, how the intercept and missing rows are treated, the iteration controls, the case weights and the labels.

## Signature

```cpp
regression::QuantileOptions options;
regression::QuantileFitResult regression::quantile_fit(const Series &y, const Series &x, const regression::QuantileOptions &options = {}, Config cfg = {});
```

A default `QuantileOptions` fits the median with an intercept, robust standard errors and 95% intervals. Every example below uses twelve points whose spread grows with x, so the quantiles differ.

## Fields

### `tau`

`double`, default `0.5`.

The quantile to model, strictly between 0 and 1. The fit minimises the check loss, which charges τ per unit of positive residual and 1 − τ per unit of negative one, so the line leaves a fraction τ of the observations below it. The default 0.5 is median regression. The program fits three quantiles of the same data; the slope grows with τ because the spread does.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  for (const double tau : {0.1, 0.5, 0.9}) {
    reg::QuantileOptions options;
    options.tau = tau;
    const auto fit = reg::quantile_fit(y, x, options);
    out.writeln("tau", tau, " coefficients", fit.coefficients.items<double>());
  }
}
```

Output

```text
tau 0.10000  coefficients [1.32500, 0.62500]
tau 0.50000  coefficients [0.58000, 1.52000]
tau 0.90000  coefficients [0.66000, 1.62000]
```

### `covariance`

`regression::QuantileCovariance`, default `Robust`.

Which covariance estimator produces the standard errors: `Robust` (default) or `Iid`. See [`QuantileCovariance`](QuantileCovariance.md). It changes only the uncertainty.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.tau = 0.9;
  const auto robust = reg::quantile_fit(y, x, options);
  options.covariance = reg::QuantileCovariance::Iid;
  const auto iid = reg::quantile_fit(y, x, options);
  out.writeln("coefficients under both", robust.coefficients.items<double>(), iid.coefficients.items<double>());
  out.writeln("robust std_errors", robust.std_errors.items<double>(), " iid", iid.std_errors.items<double>());
}
```

Output

```text
coefficients under both [0.66000, 1.62000] [0.66000, 1.62000]
robust std_errors [1.99561, 0.29707]  iid [1.21275, 0.16478]
```

### `alpha`

`double`, default `0.05`.

The significance level of the confidence intervals, so the intervals have coverage 1 − alpha, built from the Student t quantile with the residual degrees of freedom. A larger alpha gives narrower intervals and changes nothing else.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  for (const double alpha : {0.05, 0.10}) {
    reg::QuantileOptions options;
    options.alpha = alpha;
    const auto fit = reg::quantile_fit(y, x, options);
    const auto ci = fit.conf_int.reshape({4}).items<double>();   // [lower0, upper0, lower1, upper1]
    out.writeln("alpha", alpha, " slope", fit.coefficients.items<double>()[1], " interval", ci[2], ci[3]);
  }
}
```

Output

```text
alpha 0.05000  slope 1.52000  interval 0.30065 2.73935
alpha 0.10000  slope 1.52000  interval 0.52813 2.51187
```

### `add_intercept`

`bool`, default `true`.

Whether a constant column is added to the design. With the default `true` the first coefficient is the intercept, named `const`; with `false` the line is forced through the origin. The program fits the median both ways.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto with = reg::quantile_fit(y, x, options);
  options.add_intercept = false;
  const auto without = reg::quantile_fit(y, x, options);
  out.writeln("with intercept   ", with.coefficient_names, with.coefficients.items<double>());
  out.writeln("without intercept", without.coefficient_names, without.coefficients.items<double>());
}
```

Output

```text
with intercept    [const, x1] [0.58000, 1.52000]
without intercept [x1] [1.61250]
```

### `drop_missing`

`bool`, default `true`.

What to do with rows that hold NaN or infinite values. With the default `true` they are left out and counted in `dropped_observations`; with `false` any non-finite value is an error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  auto values = std::vector<double>{1, 2, std::nan(""), 4, 5, 6, 7, 8, 9, 10, 11, 12};
  const auto holey = pluss::make_series(values, {12}, as_f64());
  reg::QuantileOptions options;
  const auto fit = reg::quantile_fit(y, holey, options);
  out.writeln("drop_missing=true : nobs", fit.nobs, " dropped", fit.dropped_observations);
  options.drop_missing = false;
  try {
    reg::quantile_fit(y, holey, options);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln("drop_missing=false:", text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
drop_missing=true : nobs 11  dropped 1
drop_missing=false: subsp::stats::regression::quantile_fit: non-finite values found and drop_missing=false
```

### `rcond`

`double`, default `1e-12`.

The reciprocal-condition cutoff below which a direction of the design is treated as numerically absent, so nearly collinear predictors are declared redundant and the result's `rank` falls. The program fits a design whose second column is the first plus a tiny perturbation, at the default and at a coarse cutoff.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  std::vector<double> flat;
  for (int i = 0; i < 12; ++i) flat.push_back(1.0 * (i + 1));
  for (int i = 0; i < 12; ++i) flat.push_back(1.0 * (i + 1) + 1e-5 * ((i % 3) - 1));
  const auto x2 = pluss::make_series(flat, {2, 12}, as_f64()).transpose(0, 1);
  reg::QuantileOptions options;
  const auto fine = reg::quantile_fit(y, x2, options);
  options.rcond = 1e-3;
  const auto coarse = reg::quantile_fit(y, x2, options);
  out.writeln("rcond 1e-12: rank", fine.rank, " of", fine.k_params);
  out.writeln("rcond 1e-3 : rank", coarse.rank, " of", coarse.k_params, " coefficients", coarse.coefficients.items<double>());
}
```

Output

```text
rcond 1e-12: rank 3  of 3
rcond 1e-3 : rank 2  of 3  coefficients [0.67532, 0.71235, 0.71234]
```

### `max_iter`

`std::int64_t`, default `500`.

The cap on iteratively reweighted least-squares steps. If it is reached before the coefficients settle to `tol`, the fit is returned anyway with `converged` false and a warning. The default 500 is far above what these problems need; the program allows only three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.max_iter = 3;
  const auto short_run = reg::quantile_fit(y, x, options);
  options.max_iter = 500;
  const auto full = reg::quantile_fit(y, x, options);
  out.writeln("max_iter 3  : converged", short_run.converged, " iterations", short_run.iterations, " coefficients", short_run.coefficients.items<double>());
  out.writeln("warning     :", short_run.warnings);
  out.writeln("max_iter 500: converged", full.converged, " iterations", full.iterations, " coefficients", full.coefficients.items<double>());
}
```

Output

```text
max_iter 3  : converged false  iterations 3  coefficients [0.80285, 1.29620]
warning     : [Quantile IRLS reached max_iter without satisfying tolerance.]
max_iter 500: converged true  iterations 21  coefficients [0.58000, 1.52000]
```

### `tol`

`double`, default `1e-7`.

The convergence tolerance: the iteration stops when the largest change in a coefficient is at most `tol` times one plus the largest coefficient. A loose tolerance stops early with a rougher answer; the program compares 1e-2 with the default 1e-7.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.tol = 1e-2;
  const auto loose = reg::quantile_fit(y, x, options);
  options.tol = 1e-7;
  const auto tight = reg::quantile_fit(y, x, options);
  out.writeln("tol 1e-2: iterations", loose.iterations, " coefficients", loose.coefficients.items<double>());
  out.writeln("tol 1e-7: iterations", tight.iterations, " coefficients", tight.coefficients.items<double>());
}
```

Output

```text
tol 1e-2: iterations 8  coefficients [0.66089, 1.43911]
tol 1e-7: iterations 21  coefficients [0.58000, 1.52000]
```

### `eps`

`double`, default `1e-6`.

The smallest residual magnitude used in the iteration's weights. The quantile loss is not smooth at zero, so each reweighting step divides by the absolute residual; `eps` keeps a residual that is exactly zero from dividing by zero and smooths the loss near it. The default 1e-6 reproduces the exact quantile fit; a large value distorts it, because every residual smaller than `eps` is treated as if it were `eps`. The program fits the median with three values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  for (const double eps : {1e-6, 1e-2, 1.0}) {
    reg::QuantileOptions options;
    options.eps = eps;
    const auto fit = reg::quantile_fit(y, x, options);
    out.writeln("eps", eps, " coefficients", fit.coefficients.items<double>(), " objective", fit.objective);
  }
}
```

Output

```text
eps 0.00000  coefficients [0.58000, 1.52000]  objective 14.04000
eps 0.01000  coefficients [0.57720, 1.52080]  objective 14.04160
eps 1.00000  coefficients [0.15183, 1.50671]  objective 14.52134
```

### `weights`

`Series`, default `undefined`.

Case weights, one non-negative value per row: row i counts weights[i] times in the check-function loss, the density estimate and the covariance, as if it were repeated. A weight of zero removes the row, and it is counted in `dropped_observations`. The program gives the last row weight zero and compares with the fit that leaves it out by hand.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.weights = pluss::make_series(std::vector<double>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {12}, as_f64());
  const auto weighted = reg::quantile_fit(y, x, options);
  options.weights = pluss::series::undefined();
  const auto by_hand = reg::quantile_fit(y.slice(0, 0, 11), x.slice(0, 0, 11), options);
  out.writeln("weight zero: nobs", weighted.nobs, " dropped", weighted.dropped_observations, " coefficients", weighted.coefficients.items<double>());
  out.writeln("row removed: nobs", by_hand.nobs, "         coefficients", by_hand.coefficients.items<double>());
}
```

Output

```text
weight zero: nobs 11  dropped 1  coefficients [1.85000, 0.65000]
row removed: nobs 11          coefficients [1.85000, 0.65000]
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
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  options.response_name = "price";
  const auto fit = reg::quantile_fit(y, x, options);
  out.writeln("response_name", fit.response_name);
}
```

Output

```text
response_name price
```

### `predictor_names`

`std::vector<std::string>`, default `empty`.

Labels for the predictors, one per column of `x`, used for `coefficient_names` and in the summary. The intercept is always named `const`. Without names the predictors are called `x1`, `x2`, and so on; a list of the wrong length is an error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {12}, as_f64());
  const auto y = pluss::make_series(std::vector<double>{2.1, 3.9, 3.2, 6.8, 5.1, 9.7, 6.4, 12.9, 7.7, 16.5, 8.2, 20.1}, {12}, as_f64());
  reg::QuantileOptions options;
  const auto unnamed = reg::quantile_fit(y, x, options);
  options.predictor_names = {"size"};
  const auto named = reg::quantile_fit(y, x, options);
  out.writeln("default names", unnamed.coefficient_names);
  out.writeln("given names  ", named.coefficient_names);
  options.predictor_names = {"size", "extra"};
  try {
    reg::quantile_fit(y, x, options);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
default names [const, x1]
given names   [const, size]
subsp::stats::regression::quantile_fit: predictor_names size must equal number of design columns before intercept handling
```

## See Also

[`quantile_fit`](quantile_fit.md) consumes the options, [`QuantileCovariance`](QuantileCovariance.md) defines the estimator choice, and [`QuantileFitResult`](QuantileFitResult.md) records the resolved values and the outcome.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuantileCovariance.html">
    <span class="label">Previous</span>
    <span class="title">QuantileCovariance</span>
  </a>
  <a class="next-link" href="QuantileFitResult.html">
    <span class="label">Next</span>
    <span class="title">QuantileFitResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
