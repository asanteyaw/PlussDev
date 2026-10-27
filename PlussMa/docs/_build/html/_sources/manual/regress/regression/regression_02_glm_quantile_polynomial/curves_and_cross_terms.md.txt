<!-- generated from curves_and_cross_terms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Curves and cross terms

[Back to Generalised Linear, Quantile and Polynomial Regression](../regression_02_glm_quantile_polynomial.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

namespace reg = pluss::subsp::stats::regression;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series column(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size()), 1}).to(f64); }
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series table(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }
int main() {
  std::vector<double> xs, ys;
  for (int i = 0; i < 9; ++i) { xs.push_back(i * 0.5); ys.push_back(1.0 - 2.0 * xs.back() + 0.5 * xs.back() * xs.back()); }
  std::cout << std::fixed << std::setprecision(6);
  for (std::int64_t degree : {1, 2, 3}) {
    reg::PolyOptions o;
    o.degree = degree;
    const auto fit = reg::poly_fit(vec(ys), column(xs), o);
    std::cout << "degree " << degree << ": coefficients";
    for (double c : fit.ols.coefficients.host<double>()) std::cout << " " << std::round(c * 1e6) / 1e6 + 0.0;
    std::cout << "   R-squared " << fit.ols.rsquared << "\n";
  }
  std::cout << "poly_expand of x = 2 at degree 3: ";
  for (double v : reg::poly_expand(column({2.0}), 3).host<double>()) std::cout << v << " ";
  std::cout << "\n";

  // y = 1 + x1 + 2 x2 + 0.5 x1 x2 on a grid
  std::vector<double> grid, z;
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j) { grid.push_back(i); grid.push_back(j * 0.5); z.push_back(1.0 + i + 2.0 * j * 0.5 + 0.5 * i * j * 0.5); }
  reg::PolyOptions cross;
  cross.degree = 2;
  cross.include_cross = true;
  const auto fit = reg::poly_fit(vec(z), table(grid, 25, 2), cross);
  std::cout << "two predictors with cross term, degree 2:";
  for (const auto& name : fit.ols.coefficient_names) std::cout << " " << name;
  std::cout << "\n  coefficients:";
  for (double c : fit.ols.coefficients.host<double>()) std::cout << " " << std::setprecision(4) << c;
  std::cout << "\n";
}
```

Output

```text
degree 1: coefficients -0.166667 0.000000   R-squared 0.000000
degree 2: coefficients 1.000000 -2.000000 0.500000   R-squared 1.000000
degree 3: coefficients 1.000000 -2.000000 0.500000 0.000000   R-squared 1.000000
poly_expand of x = 2 at degree 3: 2.000000 4.000000 8.000000 
two predictors with cross term, degree 2: const x1 x1^2 x2 x2^2 x1*x2
  coefficients: 1.0000 1.0000 0.0000 2.0000 0.0000 0.5000
```
Data from a parabola are recovered exactly at degree 2 and the cubic term comes out zero. A straight line fitted to the same data explains nothing (R² = 0) because the parabola is symmetric about the middle of the range, so the best line is flat. For two predictors, `include_cross` brings in the product `x1*x2`, whose coefficient 0.5 is recovered. Polynomials of high degree oscillate and extrapolate wildly: keep the degree low and compare candidate degrees with the information criteria of the fit.
