<!-- generated from reference_curves_with_quantiles.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reference curves with quantiles

[Back to Worked Problems](../regression_05_worked_problems.md)

A reference chart, such as a growth chart, shows the percentiles of a measurement as a function of age: the 10th percentile curve is the height below which a tenth of children of that age fall. Each curve is a quantile regression, and because both the centre and the spread of the measurement change with age, the curves fan out. A fitted mean with a fixed band cannot draw that chart.

`quantile_fit` fits straight lines in its predictors, so the curvature is supplied by giving it the expanded columns from `poly_expand`: age and age squared.

The data are 130 children aged 2 to 14.9 years. The median height follows 85 + 6.2·age − 0.15·age², and the spread grows with age: the disturbance is uniform on ±(0.8 + 0.25·age)·1.7 cm. Because the disturbance is uniform, the true percentile curves are known, and the fits can be checked against them.

## Seven curves

The program fits the 3rd, 10th, 25th, 50th, 75th, 90th and 97th percentile curves and reports, for each, the share of the 130 children at or below the fitted curve (which should be close to the percentile), and the largest distance between the fitted curve and the true percentile curve over ages 2 to 14.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const int n = 130;
  std::vector<double> age, height;
  for (int i = 0; i < n; ++i) {
    const double a = 2.0 + i * 0.1;
    const double u = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;
    age.push_back(a);
    height.push_back(85.0 + 6.2 * a - 0.15 * a * a + (0.8 + 0.25 * a) * 1.7 * u);
  }
  const auto x = reg::poly_expand(pluss::make_series(age, {n}, as_f64()), 2);  // columns: age and age squared
  const auto y = pluss::make_series(height, {n}, as_f64());

  out.writeln("percentile   share of children at or below   largest distance from the true curve (cm)   iterations");
  for (double tau : {0.03, 0.10, 0.25, 0.50, 0.75, 0.90, 0.97}) {
    reg::QuantileOptions options;
    options.tau = tau;
    options.predictor_names = {"age", "age^2"};
    const auto fit = reg::quantile_fit(y, x, options);
    const auto b = fit.coefficients.items<double>();
    const double share = y.le(fit.fitted_values.add(1e-6)).to(pluss::ScalarType::Float64).mean().item<double>();
    double worst = 0.0;
    for (double a = 2.0; a <= 14.0; a += 0.5) {
      const double fitted = b[0] + b[1] * a + b[2] * a * a;
      const double truth = 85.0 + 6.2 * a - 0.15 * a * a + (0.8 + 0.25 * a) * 1.7 * (2.0 * tau - 1.0);
      worst = std::max(worst, std::abs(fitted - truth));
    }
    out.writeln(tau, share, worst, fit.iterations);
  }
}
```

Output

```text
percentile   share of children at or below   largest distance from the true curve (cm)   iterations
0.030 0.038 0.163 77
0.100 0.108 0.442 124
0.250 0.262 0.260 93
0.500 0.508 0.149 13
0.750 0.754 0.267 31
0.900 0.900 0.326 48
0.970 0.969 0.649 28
```

Each curve has close to its intended share of children at or below it: 0.038 for the 3rd percentile, 0.108, 0.262, 0.508, 0.754, 0.900 and 0.969 for the 97th. Each also lies within 0.65 cm of the true percentile curve over ages 2 to 14, with the largest distances in the tails (0.44 cm at the 10th and 0.65 cm at the 97th), where the data are sparsest. The central curves converge in fewer iterations (13 for the median) than the outer ones (77 and 124 for the 3rd and 10th).

## Locating a child

With the curves fitted, a child is placed by comparing their height with the curves at their age. The program fits the same seven curves, evaluates all of them with `quantile_predict` at age 9, checks that they are in order there, and reports the band into which three heights fall. It also contrasts the chart with the one that a least-squares fit would give: the mean curve plus or minus 1.96 residual standard deviations has a constant width, so it is too wide for young children and too narrow for old ones.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const int n = 130;
  std::vector<double> age, height;
  for (int i = 0; i < n; ++i) {
    const double a = 2.0 + i * 0.1;
    const double u = 2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0;
    age.push_back(a);
    height.push_back(85.0 + 6.2 * a - 0.15 * a * a + (0.8 + 0.25 * a) * 1.7 * u);
  }
  const auto x = reg::poly_expand(pluss::make_series(age, {n}, as_f64()), 2);
  const auto y = pluss::make_series(height, {n}, as_f64());
  const std::vector<double> percentiles{0.03, 0.10, 0.25, 0.50, 0.75, 0.90, 0.97};

  std::vector<reg::QuantileFitResult> curves;
  for (double tau : percentiles) {
    reg::QuantileOptions options;
    options.tau = tau;
    curves.push_back(reg::quantile_fit(y, x, options));
  }
  auto curve_at = [&](std::size_t k, double a) {
    return reg::quantile_predict(curves[k], pluss::make_series(std::vector<double>{a, a * a}, {1, 2}, as_f64())).items<double>()[0];
  };

  out.writeln("height by percentile at ages 3, 9 and 14:");
  for (double a : {3.0, 9.0, 14.0}) {
    std::vector<double> heights;
    for (std::size_t k = 0; k < percentiles.size(); ++k) {
      heights.push_back(curve_at(k, a));
    }
    out.writeln("   age", a, heights, " width of the 3rd-97th band", heights.back() - heights.front());
  }

  for (double measured : {118.0, 131.0, 142.0}) {
    std::size_t below = 0;
    while (below < percentiles.size() && curve_at(below, 9.0) < measured) {
      ++below;
    }
    if (below == 0) {
      out.writeln("a child aged 9 with height", measured, "is below the 3rd percentile curve");
    } else if (below == percentiles.size()) {
      out.writeln("a child aged 9 with height", measured, "is above the 97th percentile curve");
    } else {
      out.writeln("a child aged 9 with height", measured, "lies between the", percentiles[below - 1], "and", percentiles[below], "curves");
    }
  }

  const auto mean_fit = reg::ols_fit(y, x);
  out.writeln("least-squares band (mean +/- 1.96 residual sd) has the same width at every age:", 2.0 * 1.96 * mean_fit.rmse, "cm; the quantile band is", curve_at(6, 3.0) - curve_at(0, 3.0), "at age 3 and", curve_at(6, 14.0) - curve_at(0, 14.0), "at 14");
}
```

Output

```text
height by percentile at ages 3, 9 and 14:
   age 3.00 [99.67, 99.86, 100.89, 102.14, 103.40, 104.23, 104.62]  width of the 3rd-97th band 4.95
   age 9.00 [123.74, 124.59, 126.02, 128.63, 131.29, 132.92, 133.57]  width of the 3rd-97th band 9.84
   age 14.00 [135.37, 136.15, 138.49, 142.28, 145.79, 147.92, 148.62]  width of the 3rd-97th band 13.26
a child aged 9 with height 118.00 is below the 3rd percentile curve
a child aged 9 with height 131.00 lies between the 0.50 and 0.75 curves
a child aged 9 with height 142.00 is above the 97th percentile curve
least-squares band (mean +/- 1.96 residual sd) has the same width at every age: 11.91 cm; the quantile band is 4.95 at age 3 and 13.26 at 14
```

The 3rd to 97th percentile band is 4.95 cm wide at age 3, 9.84 cm at age 9 and 13.26 cm at age 14: the fan that a growth chart shows. A child aged 9 who is 118 cm tall is below the 3rd percentile curve, one of 131 cm lies between the 50th and 75th, and one of 142 cm is above the 97th. The least-squares band (mean ± 1.96 residual standard deviations) is 11.91 cm wide at every age, which is 2.4 times too wide at age 3 and about 10% too narrow at age 14.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="comparing_two_event_rates.html">
    <span class="label">Previous</span>
    <span class="title">Comparing two event rates</span>
  </a>
  <a class="next-link" href="the_same_data_four_ways.html">
    <span class="label">Next</span>
    <span class="title">The same data, four ways</span>
  </a>
</div>
<!-- apiref:nav:end -->
