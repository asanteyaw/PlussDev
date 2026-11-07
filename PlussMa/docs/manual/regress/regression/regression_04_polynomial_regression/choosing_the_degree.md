<!-- generated from choosing_the_degree.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Choosing the degree

[Back to Polynomial Regression](../regression_04_polynomial_regression.md)

Raising the degree always lowers the sum of squared residuals, because each model contains the one below it. So R² can only go up, and it cannot choose the degree. What can is a measure that charges for each added coefficient, or the error on data the model has not seen.

| Measure | Behaviour as the degree rises |
|---|---|
| `rsquared` | never decreases |
| `aic`, `bic` | fall, then rise once an extra term no longer pays for itself; `bic` charges more for each term than `aic` |
| `adj_rsquared` | penalises each term, but lightly: it can keep creeping up at a high degree |
| error on held-out data | falls, then rises when the curve starts following the noise |

## Fitting degrees 1 to 8

The data are a quadratic, y = 2 + 1.5x − 0.12x², observed at 30 points with noise. The program holds out every fifth point, fits degrees 1 to 8 to the other 24, and reports the criteria and the root-mean-square error on the six held-out points (these lie inside the range of the fitted data, so this tests interpolation).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  std::vector<double> train_x, train_y, test_x, test_y;
  for (int i = 0; i < 30; ++i) {
    const double x = i * 0.5;
    const double noise = 0.8 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0);
    const double y = 2.0 + 1.5 * x - 0.12 * x * x + noise;
    (i % 5 == 2 ? test_x : train_x).push_back(x);
    (i % 5 == 2 ? test_y : train_y).push_back(y);
  }
  const auto x = pluss::make_series(train_x, {24}, as_f64());
  const auto y = pluss::make_series(train_y, {24}, as_f64());
  const auto held_x = pluss::make_series(test_x, {6}, as_f64());
  const auto held_y = pluss::make_series(test_y, {6}, as_f64());

  out.writeln("degree     R2   adj R2      AIC       BIC   train rmse   held-out rmse");
  for (std::int64_t degree = 1; degree <= 8; ++degree) {
    reg::PolyOptions options;
    options.degree = degree;
    const auto fit = reg::poly_fit(y, x, options);
    const double held_out = std::sqrt(reg::poly_predict(fit, held_x).sub(held_y).pow(2.0).mean().item<double>());
    out.writeln(degree, fit.ols.rsquared, fit.ols.adj_rsquared, fit.ols.aic, fit.ols.bic, fit.ols.rmse, held_out);
  }
}
```

Output

```text
degree     R2   adj R2      AIC       BIC   train rmse   held-out rmse
1 0.1853 0.1483 108.4996 110.8557 2.2292 1.8841
2 0.9529 0.9484 42.0740 45.6081 0.5485 0.2347
3 0.9545 0.9476 43.2801 47.9923 0.5528 0.2599
4 0.9547 0.9451 45.1746 51.0649 0.5659 0.2607
5 0.9594 0.9481 44.5409 51.6092 0.5504 0.3256
6 0.9607 0.9468 45.7615 54.0079 0.5572 0.3459
7 0.9657 0.9507 44.4556 53.8801 0.5361 0.5085
8 0.9662 0.9482 46.1205 56.7230 0.5499 0.5059
```

R² rises with every degree, as it must, from 0.185 for a straight line to 0.966 for degree 8, and says nothing about where to stop. AIC and BIC are lowest at degree 2 (42.07 and 45.61) and rise afterwards, and the held-out error is lowest there too (0.235) and roughly doubles by degree 7. The adjusted R² is the weakest guide: it is highest at degree 7 (0.9507, against 0.9484 at degree 2), because its penalty for an extra term is small. The training error does not improve beyond degree 2 (0.549) while the curve fits the held-out points progressively worse: the higher-degree curves are following the noise.

## Conditioning

The columns x, x², x³, ... of a predictor that is far from zero or has a narrow range are nearly proportional to one another, so the design becomes ill conditioned long before the degree is high. The condition number in the result measures it, and the `warnings` say when it matters. A rank below the number of parameters means the design has lost directions, and the coefficients are then the minimum-norm ones ([Collinear predictors](../regression_01_least_squares/collinear_predictors.md)).

Centring and scaling the predictor before the fit removes most of the problem and does not change the fitted curve: the same data are fitted below with the raw predictor, x from 100 to 119, and with the predictor centred and divided by its range.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace reg = pluss::subsp::stats::regression;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  std::vector<double> xs, ys;
  for (int i = 0; i < 20; ++i) {
    const double x = 100.0 + i;
    xs.push_back(x);
    ys.push_back(1.0 + 0.01 * x + 0.0001 * x * x + 0.05 * (2.0 * std::fmod(i * 0.6180339887498949, 1.0) - 1.0));
  }
  const auto raw = pluss::make_series(xs, {20}, as_f64());
  const auto y = pluss::make_series(ys, {20}, as_f64());
  const auto scaled = raw.sub(109.5).div(19.0);  // centred, and in units of the range

  out.writeln("degree  rank/k (raw)  condition (raw)  warnings (raw)  rank/k (scaled)  condition (scaled)  R2 raw  R2 scaled");
  for (std::int64_t degree : {1, 2, 3, 4, 5, 6}) {
    reg::PolyOptions options;
    options.degree = degree;
    const auto a = reg::poly_fit(y, raw, options);
    const auto b = reg::poly_fit(y, scaled, options);
    out.writeln(degree, a.ols.rank, "/", a.ols.k_params, a.ols.condition_number, a.ols.warnings.size(), b.ols.rank, "/", b.ols.k_params, b.ols.condition_number, a.ols.rsquared, b.ols.rsquared);
  }
}
```

Output

```text
degree  rank/k (raw)  condition (raw)  warnings (raw)  rank/k (scaled)  condition (scaled)  R2 raw  R2 scaled
1 2 / 2 2.09e+03 1 2 / 2 3.30e+00 9.77e-01 9.77e-01
2 3 / 3 4.89e+06 1 3 / 3 1.23e+01 9.77e-01 9.77e-01
3 4 / 4 1.18e+10 1 4 / 4 4.70e+01 9.78e-01 9.78e-01
4 4 / 5 2.91e+13 2 5 / 5 1.83e+02 9.78e-01 9.78e-01
5 4 / 6 7.32e+16 2 6 / 6 7.28e+02 9.78e-01 9.79e-01
6 4 / 7 1.88e+20 2 7 / 7 2.95e+03 9.78e-01 9.79e-01
```

With x between 100 and 119 the raw design is ill conditioned from the start. The condition number is 2,090 for a straight line, already above the warning threshold of 1000, and it grows by a factor of about two and a half thousand with each degree. From degree 4 the rank is 4 whatever the number of parameters (4 of 5, 4 of 6, 4 of 7), so the design has lost directions and the coefficients are the minimum-norm ones. After centring and scaling the condition numbers are 3.3, 12, 47, 183, 728 and 2,950: the design has full rank at every degree, and the R² values are the same to the digits shown (0.977 to 0.979).

The same fitted curve is obtained from any such linear change of the predictor, so it is safe to fit on the scaled variable and to predict with the same transformation.
