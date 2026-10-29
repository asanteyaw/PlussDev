<!-- generated from several_quantiles.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Several quantiles

[Back to Quantile Regression](../regression_03_quantile_regression.md)

A single mean line summarises where the response is centred. A family of quantile lines, for example τ = 0.1, 0.5 and 0.9, describes how the whole conditional distribution changes with the predictors: its centre, its spread, and whether it is skewed. When the spread of the response grows with a predictor, the lines fan out. Least squares, which models only the mean, cannot show that.

## A fan of lines

The response is y = 1 + 2x + x·u, where u is a fixed sequence of values spread evenly between −1 and 1, so the noise grows with x (this is called *heteroscedasticity*). The program fits five quantile lines. For each it reports the coefficients, how many residuals lie strictly below the line (this should be about τ of the 60 observations), the check-function loss and the pseudo R². The pseudo R² compares the loss with the loss of a constant at the τ-quantile of the response.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> xs, ys;
  for (int i = 0; i < 60; ++i) {
    const double x = 1.0 + i * 0.25;
    xs.push_back(x);
    const double noise = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;  // a fixed low-discrepancy sequence in [-1, 1)
    ys.push_back(1.0 + 2.0 * x + x * noise);
  }
  const auto x = pluss::make_series(xs, {60, 1}, as_f64());
  const auto y = pluss::make_series(ys, {60}, as_f64());

  const auto ols = reg::ols_fit(y, x);
  out.writeln("least squares (the mean): intercept and slope", ols.coefficients.pull<double>());
  out.writeln("   tau   intercept and slope    below   objective   pseudo R2   converged");
  for (double tau : {0.05, 0.25, 0.5, 0.75, 0.95}) {
    reg::QuantileOptions options;
    options.tau = tau;
    const auto fit = reg::quantile_fit(y, x, options);
    const auto below = static_cast<std::int64_t>(fit.residuals.lt(-1e-9).to(pluss::ScalarType::Float64).sum().item<double>());
    out.writeln(tau, fit.coefficients.pull<double>(), below, "of", fit.nobs, fit.objective, fit.pseudo_rsquared, fit.converged);
  }
}
```

Output

```text
least squares (the mean): intercept and slope [0.3965, 2.0958]
   tau   intercept and slope    below   objective   pseudo R2   converged
0.0500 [0.8967, 1.1033] 4 of 60 24.1068 0.4641 true
0.2500 [0.3864, 1.6136] 15 of 60 93.2843 0.4820 true
0.5000 [0.7765, 2.0557] 30 of 60 123.6668 0.5201 true
0.7500 [0.6953, 2.5383] 44 of 60 93.2019 0.5798 true
0.9500 [0.8932, 2.9241] 56 of 60 23.6032 0.6755 true
```

The slope rises with τ: 1.10, 1.61, 2.06, 2.54 and 2.92. The 5% line is nearly flat in comparison with the 95% line, a fan that opens to the right, which is the pattern of noise that grows with x. The least-squares line, [0.3965, 2.0958], is close to the median line because the noise is symmetric. The count of observations strictly below each line is within one of τn (3, 15, 30, 45 and 57): each line passes through two observations, and those lie on it and not below it. The pseudo R² values are between 0.46 and 0.68.

## Reading the fan

The slope of the quantile line is the rate at which that quantile of the response rises with x. When the slopes differ across τ, the distribution of the response is changing shape and not only location. The width between the 5% and 95% lines is a measure of the conditional spread, and here it is proportional to x. The program evaluates the fitted lines at three values of x and prints the width of the band between the outer lines. It also checks that the quantile lines do not cross at those points: a lower quantile should never be above a higher one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> xs, ys;
  for (int i = 0; i < 60; ++i) {
    const double x = 1.0 + i * 0.25;
    xs.push_back(x);
    const double noise = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;  // a fixed low-discrepancy sequence in [-1, 1)
    ys.push_back(1.0 + 2.0 * x + x * noise);
  }
  const auto x = pluss::make_series(xs, {60, 1}, as_f64());
  const auto y = pluss::make_series(ys, {60}, as_f64());

  const std::vector<double> taus{0.05, 0.25, 0.5, 0.75, 0.95};
  const auto at = pluss::make_series(std::vector<double>{2.0, 8.0, 15.0}, {3, 1}, as_f64());
  std::vector<std::vector<double>> lines;
  for (double tau : taus) {
    reg::QuantileOptions options;
    options.tau = tau;
    lines.push_back(reg::quantile_predict(reg::quantile_fit(y, x, options), at).pull<double>());
  }
  out.writeln("x                 ", at.reshape({3}).pull<double>());
  for (std::size_t i = 0; i < taus.size(); ++i) {
    out.writeln("quantile line", taus[i], lines[i]);
  }
  std::vector<double> width;
  bool ordered = true;
  for (std::size_t j = 0; j < 3; ++j) {
    width.push_back(lines[4][j] - lines[0][j]);
    for (std::size_t i = 1; i < taus.size(); ++i) {
      ordered = ordered && lines[i][j] >= lines[i - 1][j];
    }
  }
  out.writeln("width of the 5%-95% band", width, " lines in order at every point:", ordered ? "yes" : "no");
}
```

Output

```text
x                  [2.0000, 8.0000, 15.0000]
quantile line 0.0500 [3.1033, 9.7231, 17.4462]
quantile line 0.2500 [3.6136, 13.2955, 24.5910]
quantile line 0.5000 [4.8880, 17.2223, 31.6123]
quantile line 0.7500 [5.7719, 21.0016, 38.7695]
quantile line 0.9500 [6.7415, 24.2863, 44.7553]
width of the 5%-95% band [3.6382, 14.5633, 27.3092]  lines in order at every point: yes
```

The five lines are in order at all three points, and the width of the band between the 5% and 95% lines is 3.64, 14.56 and 27.31, proportional to x (it is 1.82 x in each case), as the construction of the data predicts.

Nothing in the method forces separately fitted quantile lines to stay in order, and with little data or far from the centre they can cross. A crossing means the data do not support that pair of lines at that point; do not read a quantile function off lines that cross.
