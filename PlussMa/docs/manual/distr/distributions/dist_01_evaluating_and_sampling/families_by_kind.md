<!-- generated from families_by_kind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Families by kind

[Back to Evaluating and Sampling](../dist_01_evaluating_and_sampling.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
namespace dst = pluss::subsp::stats::distr::dst;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(30) << label << std::right;
  for (double v : s.to_vec<double>()) std::cout << std::setw(10) << v;
  std::cout << "\n";
}
int main() {
  std::cout << std::fixed << std::setprecision(5);
  const std::vector<double> t{0.5, 1.0, 2.0, 4.0};
  const Series x = vec(t);
  row("x", x);
  row("Weibull(1.7, 2.1) pdf", distr::pdf(dst::Weibull, x, {1.7, 2.1}));
  row("Weibull(1.7, 2.1) cdf", distr::cdf(dst::Weibull, x, {1.7, 2.1}));
  row("Student t(4.5) pdf", distr::pdf(dst::StudentT, x, {4.5}));
  row("Lomax(1.5, 2.5) pdf", distr::pdf(dst::Lomax, x, {1.5, 2.5}));
  row("Rayleigh(1.4) pdf", distr::pdf(dst::Rayleigh, x, {1.4}));
  row("Gompertz(0.8, 2.0) pdf", distr::pdf(dst::Gompertz, x, {0.8, 2.0}));

  // Chi-squared is a gamma: the same numbers from two kinds.
  row("ChiSquare(5) cdf", distr::cdf(dst::ChiSquare, x, {5.0}));
  row("Gamma(2.5, 2, 0) cdf", distr::cdf(dst::Gamma, x, {0.0, 2.0, 2.5}));

  // The density integrates to the cdf: trapezoid of the density of a fine grid against cdf(2) - cdf(0.5) of the Weibull.
  std::vector<double> grid;
  for (int i = 0; i <= 3000; ++i) grid.push_back(0.5 + 1.5 * i / 3000.0);
  const auto dens = distr::pdf(dst::Weibull, vec(grid), {1.7, 2.1}).to_vec<double>();
  double area = 0.0;
  for (std::size_t i = 1; i < grid.size(); ++i) area += 0.5 * (dens[i] + dens[i - 1]) * (grid[i] - grid[i - 1]);
  const auto c = distr::cdf(dst::Weibull, vec({0.5, 2.0}), {1.7, 2.1}).to_vec<double>();
  std::cout << "\nintegral of the Weibull density over [0.5, 2] " << area << "   cdf(2) - cdf(0.5) " << c[1] - c[0] << "\n";

  // Samples by kind.
  const Series draws = distr::sample(dst::Gamma, {0.0, 1.0, 3.0}, {100000}, pluss::Config{}.with_dtype(f64), 5ULL);
  std::cout << "mean of 100000 draws of Gamma(3, scale 1) " << draws.mean().item<double>() << "   (exact 3)\n";
  std::cout << "kinds implemented: Normal " << distr::supports(dst::Normal) << ", Gh " << distr::supports(dst::Gh) << "\n";
}
```

Output

```text
x                                0.50000   1.00000   2.00000   4.00000
Weibull(1.7, 2.1) pdf            0.27170   0.36278   0.31165   0.06389
Weibull(1.7, 2.1) cdf            0.08350   0.24670   0.60164   0.94973
Student t(4.5) pdf               0.32539   0.21742   0.06568   0.00583
Lomax(1.5, 2.5) pdf              0.60892   0.27885   0.08589   0.01766
Rayleigh(1.4) pdf                0.23934   0.39533   0.36780   0.03445
Gompertz(0.8, 2.0) pdf           0.70143   0.50452   0.10033   0.00000
ChiSquare(5) cdf                 0.00788   0.03743   0.15085   0.45058
Gamma(2.5, 2, 0) cdf             0.00788   0.03743   0.15085   0.45058

integral of the Weibull density over [0.5, 2] 0.51814   cdf(2) - cdf(0.5) 0.51814
mean of 100000 draws of Gamma(3, scale 1) 3.00699   (exact 3)
kinds implemented: Normal 1, Gh 1
```
The kind-based functions give the same numbers as the objects, and relations between families can be checked directly: a chi-squared with 5 degrees of freedom is a gamma with shape 2.5 and scale 2 (the two rows are identical), and the integral of the Weibull density over [0.5, 2], by the trapezoidal rule on 3000 steps, equals `cdf(2) − cdf(0.5)` (both 0.51814). The mean of draws from a gamma of shape 3 and scale 1 is 3.007, close to the exact 3.
