<!-- generated from case_weights.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Case weights

[Back to Quantile Regression](../regression_03_quantile_regression.md)

`QuantileOptions::weights` gives each row a non-negative weight. Row *i* counts `weights[i]` times, exactly as if it were repeated, in every part of the fit: the check-function loss, the null loss, the estimate of the density of the residuals at zero, and the covariance of the coefficients. The weighted line is therefore the weighted quantile line: the line below which a fraction τ of the *weight* lies.

| Quantity | With weights |
|---|---|
| `coefficients` | the minimiser of Σ wᵢ ρ_τ(rᵢ) |
| `objective`, `objective_null` | the weighted loss at the fit and at the weighted sample τ-quantile of `y` |
| `residual_density0` | the kernel estimate with the weights as frequencies; its bandwidth uses the total weight in place of the row count |
| `covariance` | the `Iid` form τ(1 − τ)(X′WX)⁻¹ / f₀², or the sandwich with the weights in X′WX and in the middle |
| `nobs`, `df_resid` | the rows used (not the total weight), and rows less rank |
| `residuals`, `fitted_values` | `y − Xb` and `Xb`, per row |

A row of weight zero is not used and is counted in `dropped_observations`. Negative or non-finite-where-kept weights, and weights of the wrong length, are errors, as in the least-squares chapter ([Case weights](../regression_01_least_squares/case_weights.md)).

## Integer weights are repeated rows

The program fits the median and the 0.8-quantile lines to twelve rows with weights 1 to 3, and to the 24-row data set in which each row is repeated. The coefficients, objectives, density at zero and standard errors agree. The weighted quantile of the response used for the null model is the one of the repeated sample.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  std::vector<double> xs, ys, counts, repeated_x, repeated_y;
  for (int i = 0; i < 12; ++i) {
    xs.push_back(i);
    ys.push_back(1.0 + 0.7 * i + 1.5 * (2.0 * std::fmod((i + 1) * 0.6180339887498949, 1.0) - 1.0));
    counts.push_back(1.0 + (i % 3));
  }
  for (std::size_t i = 0; i < xs.size(); ++i) {
    for (int c = 0; c < static_cast<int>(counts[i]); ++c) {
      repeated_x.push_back(xs[i]);
      repeated_y.push_back(ys[i]);
    }
  }
  const auto n_repeated = static_cast<std::int64_t>(repeated_x.size());
  for (double tau : {0.5, 0.8}) {
    reg::QuantileOptions weighted;
    weighted.tau = tau;
    weighted.tol = 1e-10;
    weighted.max_iter = 2000;
    weighted.weights = pluss::make_series(counts, {12}, as_f64());
    reg::QuantileOptions plain = weighted;
    plain.weights = pluss::series::undefined();
    const auto a = reg::quantile_fit(pluss::make_series(ys, {12}, as_f64()), pluss::make_series(xs, {12}, as_f64()), weighted);
    const auto b = reg::quantile_fit(pluss::make_series(repeated_y, {n_repeated}, as_f64()), pluss::make_series(repeated_x, {n_repeated}, as_f64()), plain);
    out.writeln("tau", tau, " rows", a.nobs, "and", b.nobs);
    out.writeln("  coefficients weighted", a.coefficients.pull<double>(), " repeated", b.coefficients.pull<double>());
    out.writeln("  objective    weighted", a.objective, " repeated", b.objective, "  null", a.objective_null, b.objective_null);
    out.writeln("  density at 0 weighted", a.residual_density0, " repeated", b.residual_density0);
    out.writeln("  std errors   weighted", a.std_errors.pull<double>(), " repeated", b.std_errors.pull<double>());
  }
}
```

Output

```text
tau 0.50000  rows 12 and 24
  coefficients weighted [1.35410, 0.67910]  repeated [1.35410, 0.67910]
  objective    weighted 7.87500  repeated 7.87500   null 26.39357 26.39357
  density at 0 weighted 0.33669  repeated 0.33669
  std errors   weighted [0.59729, 0.08823]  repeated [0.59729, 0.08823]
tau 0.80000  rows 12 and 24
  coefficients weighted [2.10410, 0.67910]  repeated [2.10410, 0.67910]
  objective    weighted 4.65000  repeated 4.65000   null 16.37949 16.37949
  density at 0 weighted 0.24195  repeated 0.24195
  std errors   weighted [0.77715, 0.11323]  repeated [0.77715, 0.11323]
```

Every quantity agrees between the 12 weighted rows and the 24 repeated rows: the coefficients (1.3541 and 0.6791 at the median, 2.1041 and 0.6791 at τ = 0.8), the objectives and null objectives, the density of the residuals at zero, and the standard errors.

## Weights that matter: survey-style data

A typical use is data in which rows stand for different numbers of units: group means with group sizes, or a sample whose strata are over- or under-represented. The program has two groups, a large one near y = 10 and a small one near y = 30, and shows that the unweighted median line follows the row counts while the weighted median follows the weights, the share of the underlying units.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Eight rows: five describe a group of 20 units each at about 10, three describe groups of 1 unit each at about 30.
  const auto y = pluss::make_series(std::vector<double>{9.5, 10.2, 9.8, 10.4, 10.1, 29.0, 30.5, 31.2}, {8}, as_f64());
  const auto none = pluss::series::zeros({8, 0}, as_f64());
  const std::vector<double> units{20, 20, 20, 20, 20, 1, 1, 1};

  reg::QuantileOptions median;
  const auto by_rows = reg::quantile_fit(y, none, median);
  median.weights = pluss::make_series(units, {8}, as_f64());
  const auto by_units = reg::quantile_fit(y, none, median);
  out.writeln("median of the 8 rows    ", by_rows.coefficients.pull<double>(), " converged", by_rows.converged);
  out.writeln("median of the 103 units ", by_units.coefficients.pull<double>(), " converged", by_units.converged);

  reg::QuantileOptions top = median;
  top.tau = 0.99;
  out.writeln("99th percentile of the units", reg::quantile_fit(y, none, top).coefficients.pull<double>());
}
```

Output

```text
median of the 8 rows     [10.3829]  converged true
median of the 103 units  [10.1000]  converged true
99th percentile of the units [30.5000]
```

The median of the eight rows is 10.38, pulled up by the three high rows that are a minority of the units but three eighths of the rows. The median of the 103 units, with the rows weighted by the units they stand for, is 10.10, the middle of the large group, and the 99th percentile of the units, 30.5, lies among the three single units of the small group.
