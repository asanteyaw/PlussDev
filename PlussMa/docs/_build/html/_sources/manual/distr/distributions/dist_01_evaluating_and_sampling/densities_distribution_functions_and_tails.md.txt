<!-- generated from densities_distribution_functions_and_tails.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Densities, distribution functions and tails

[Back to Evaluating and Sampling](../dist_01_evaluating_and_sampling.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(26) << label << std::right;
  for (double v : s.to_vec<double>()) std::cout << std::setw(12) << v;
  std::cout << "\n";
}
int main() {
  std::cout << std::fixed << std::setprecision(5);
  const Series x = vec({-2.0, -1.0, 0.0, 1.0, 2.0});
  const distr::Normal normal{0.0, 1.0};
  row("x", x);
  row("normal pdf", normal.pdf(x));
  row("normal cdf", normal.cdf(x));
  row("normal logpdf", normal.logpdf(x));

  // A discrete family: the density is the probability mass at the integers.
  const Series counts = vec({0, 1, 2, 3, 4, 5});
  row("\ncounts", counts);
  row("Poisson(2.5) pmf", distr::Poisson{2.5}.pdf(counts));
  row("Poisson(2.5) cdf", distr::Poisson{2.5}.cdf(counts));

  // Far in the tail the plain cdf underflows to exactly 0 (below about 1e-308), the logarithm does not.
  const Series far = vec({-10.0, -20.0, -38.0, -45.0, -60.0});
  std::cout << std::scientific << std::setprecision(4);
  row("\nfar tail x", far);
  row("normal cdf", normal.cdf(far));
  std::cout << std::fixed << std::setprecision(3);
  row("normal logcdf", normal.logcdf(far));

  // One distribution per point: the mean and standard deviation are Series too.
  const distr::Normal each{vec({0.0, 1.0, 2.0, 3.0}), vec({1.0, 1.0, 2.0, 2.0})};
  std::cout << std::setprecision(5);
  row("\npoints (-1, 0, 1, 2) under N(0,1), N(1,1), N(2,4), N(3,4)", each.pdf(vec({-1.0, 0.0, 1.0, 2.0})));
}
```

Output

```text
x                             -2.00000    -1.00000     0.00000     1.00000     2.00000
normal pdf                     0.05399     0.24197     0.39894     0.24197     0.05399
normal cdf                     0.02275     0.15866     0.50000     0.84134     0.97725
normal logpdf                 -2.91894    -1.41894    -0.91894    -1.41894    -2.91894

counts                        0.00000     1.00000     2.00000     3.00000     4.00000     5.00000
Poisson(2.5) pmf               0.08208     0.20521     0.25652     0.21376     0.13360     0.06680
Poisson(2.5) cdf               0.08208     0.28730     0.54381     0.75758     0.89118     0.95798

far tail x                -1.0000e+01 -2.0000e+01 -3.8000e+01 -4.5000e+01 -6.0000e+01
normal cdf                  7.6199e-24  2.7536e-89 2.8854e-316  0.0000e+00  0.0000e+00
normal logcdf                  -53.231    -203.917    -726.557   -1017.226   -1805.014

points (-1, 0, 1, 2) under N(0,1), N(1,1), N(2,4), N(3,4)     0.24197     0.24197     0.17603     0.17603
```
The standard normal density at 0 is 0.39894 and its distribution function at 1 is 0.84134, as in any table. The Poisson mass and cdf are the probabilities of exactly and at most `k` events. In the far tail the plain `cdf` shrinks to 10⁻³¹⁶ at −38 and then to exactly 0, while `logcdf` goes on smoothly (−726.6 at −38, −1805.0 at −60), the quantity a likelihood or a tail-risk calculation needs. The last line evaluates four different normals at four points in one call.
