<!-- generated from a_calibration_line_and_its_uncertainty.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A calibration line and its uncertainty

[Back to Worked Problems](../regression_05_worked_problems.md)

An instrument is calibrated by measuring ten standards of known concentration. The fitted line is then used backwards: a reading is taken on an unknown sample, and the line is inverted to estimate its concentration. This problem uses `ols_fit`, its covariance matrix and a *t* distribution to answer three questions: does the instrument respond proportionally to concentration, what concentration does a new reading imply, and how uncertain is that estimate?

The standards are 0, 1, ..., 9 units and the readings follow y = 0.12 + 0.52x with a small disturbance.

## Is the response what the maker says?

The maker claims a sensitivity of 0.50 readings per unit. The test statistic for the hypothesis that the slope is 0.50 is (b₁ − 0.50) / se(b₁), with the residual degrees of freedom. The program also tests whether the intercept is zero (no blank response), which is the statistic printed in the table of the fit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  std::vector<double> standards, readings;
  for (int i = 0; i < 10; ++i) {
    standards.push_back(i);
    readings.push_back(0.12 + 0.52 * i + 0.05 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0));
  }
  const auto x = pluss::make_series(standards, {10}, as_f64());
  const auto y = pluss::make_series(readings, {10}, as_f64());
  reg::OLSOptions options;
  options.response_name = "reading";
  options.predictor_names = {"concentration"};
  const auto fit = reg::ols_fit(y, x, options);

  const auto b = fit.coefficients.pull<double>();
  const auto se = fit.std_errors.pull<double>();
  const distr::StudentT t(static_cast<double>(fit.df_resid), 0.0, 1.0);
  auto two_sided = [&](double statistic) {
    return 1.0 - t.cdf(pluss::make_series(std::vector<double>{std::abs(statistic)}, {1}, as_f64())).pull<double>()[0];
  };
  out.writeln("slope", b[1], "+/-", se[1], " residual sd", fit.rmse, " R2", fit.rsquared);
  const double stat = (b[1] - 0.50) / se[1];
  out.writeln("H0: slope = 0.50  t =", stat, " p =", 2.0 * two_sided(stat));
  out.writeln("H0: intercept = 0 t =", fit.t_values.pull<double>()[0], " p =", fit.p_values.pull<double>()[0], "(the fit's own table)");
  const auto ci = fit.conf_int.host<double>().array().data;
  out.writeln("95% interval of the slope", ci[2], ci[3], "contains 0.50:", (ci[2] <= 0.5 && 0.5 <= ci[3]) ? "yes" : "no");
}
```

Output

```text
slope 0.52423 +/- 0.00337  residual sd 0.03065  R2 0.99967
H0: slope = 0.50  t = 7.17983  p = 0.00009
H0: intercept = 0 t = 5.50066  p = 0.00057 (the fit's own table)
95% interval of the slope 0.51645 0.53201 contains 0.50: no
```

The slope is 0.5242 ± 0.0034 with a residual standard deviation of 0.031 and an R² of 0.9997. The hypothesis of a slope of 0.50 is rejected (t = 7.18, p = 0.00009), and the interval for the slope, [0.5165, 0.5320], excludes 0.50, so the instrument is slightly more sensitive than claimed. The intercept, about 0.12, is clearly different from zero (t = 5.50, p = 0.0006): there is a blank response, so the calibration must keep the intercept, and a line forced through the origin would bias every estimate.

## Inverting the line

A reading y₀ = 2.90 is taken on the unknown. The estimated concentration is x̂₀ = (y₀ − b₀) / b₁. Its variance is, to first order (the *delta method*), the variance of the reading error, `mse`, plus the variance of b₀ + b₁x̂₀ from the covariance matrix, all divided by b₁²:

Var(x̂₀) ≈ [ s² + Var(b₀) + 2 x̂₀ Cov(b₀, b₁) + x̂₀² Var(b₁) ] / b₁².

A second interval comes from inverting the 95% prediction band of the line itself: the concentrations whose predicted reading, give or take the band, can produce y₀. The two should be close in a well-behaved calibration.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> standards, readings;
  for (int i = 0; i < 10; ++i) {
    standards.push_back(i);
    readings.push_back(0.12 + 0.52 * i + 0.05 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0));
  }
  const auto fit = reg::ols_fit(pluss::make_series(readings, {10}, as_f64()), pluss::make_series(standards, {10}, as_f64()));
  const auto b = fit.coefficients.pull<double>();
  const auto cov = fit.covariance.host<double>().array().data;  // 2 x 2, row-major
  const auto ci = fit.conf_int.host<double>().array().data;
  const double multiplier = (ci[3] - ci[2]) / 2.0 / fit.std_errors.pull<double>()[1];

  const double reading = 2.90;
  const double x0 = (reading - b[0]) / b[1];
  const double variance = (fit.mse + cov[0] + 2.0 * x0 * cov[1] + x0 * x0 * cov[3]) / (b[1] * b[1]);
  out.writeln("estimated concentration", x0, " delta-method std error", std::sqrt(variance));
  out.writeln("delta-method interval  [", x0 - multiplier * std::sqrt(variance), ",", x0 + multiplier * std::sqrt(variance), "]");

  // Invert the prediction band: the band at c is  b0 + b1 c  +/-  t sqrt(mse + var(mean response at c)).
  auto band = [&](double c, double sign) {
    const double mean_var = cov[0] + 2.0 * c * cov[1] + c * c * cov[3];
    return b[0] + b[1] * c + sign * multiplier * std::sqrt(fit.mse + mean_var);
  };
  auto solve = [&](double sign) {  // the concentration c at which the band passes through the reading
    double lo = -10.0, hi = 30.0;
    for (int step = 0; step < 100; ++step) {
      const double mid = 0.5 * (lo + hi);
      (band(mid, sign) < reading ? lo : hi) = mid;
    }
    return 0.5 * (lo + hi);
  };
  out.writeln("band-inversion interval [", solve(+1.0), ",", solve(-1.0), "]");
  out.writeln("the point estimate is the midpoint only approximately:", 0.5 * (solve(+1.0) + solve(-1.0)));
}
```

Output

```text
estimated concentration 5.3429  delta-method std error 0.0616
delta-method interval  [ 5.2010 , 5.4849 ]
band-inversion interval [ 5.2011 , 5.4851 ]
the point estimate is the midpoint only approximately: 5.3431
```

The reading of 2.90 implies a concentration of 5.343, with a delta-method standard error of 0.0616 and an interval of [5.2010, 5.4849]. Inverting the prediction band gives [5.2011, 5.4851], the same to three decimals. The interval is nearly symmetric about the estimate (the midpoint of the band interval is 5.3431) because the slope is estimated very precisely (it is 155 standard errors from zero) and the reading lies in the middle of the standards. The interval would be asymmetric, and the delta method less trustworthy, for a reading near the end of the range or for a noisier line.

The intervals describe the imprecision of the line and of one reading. Averaging several readings of the same unknown shrinks the reading-error term `mse` to `mse / m`, which is the usual way to tighten a calibration estimate.
