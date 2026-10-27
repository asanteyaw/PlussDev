// Evaluating a distribution. A distribution object holds its parameters: `Normal{mean, stddev}`, `Gamma{shape, scale, location}`, `Poisson{lambda}` and so on. Its methods take a
// `Series` of points and return a `Series` of the same shape: `pdf` (density, or probability mass for a discrete family), `logpdf`, `cdf` and `logcdf`. The logarithmic forms keep their
// accuracy where the plain ones underflow: far in a tail `cdf` rounds to 0 or 1 while `logcdf` still tells how far out the point is. A parameter may itself be a Series: the points are
// then paired with the parameters element by element, one distribution per point (the Series must have the same shape as the points).
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
  for (double v : s.host<double>()) std::cout << std::setw(12) << v;
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
