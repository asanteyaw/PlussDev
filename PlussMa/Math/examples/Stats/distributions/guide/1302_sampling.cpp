// Drawing random numbers. `dist.sample({n})` returns a Series of independent draws; `sample({rows, cols}, cfg, seed)` an array of any shape; `sample_like(like)` draws an array with the
// shape, dtype and platform of an existing one. The same `seed` gives the same numbers on every run (seed 0 is a fixed default, not a random one); different seeds give different
// streams. The draws are checked here against what the distribution implies: sample mean and variance against the exact mean and variance read from the cumulants (`cumulant(1)` is the
// mean, `cumulant(2)` the variance), and the empirical probability of a region against the cdf. For a sample of n the mean is accurate to about sd / sqrt(n).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

template <class D>
void check(const char* name, const D& d, double threshold) {
  const Series draws = d.sample({200000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 20261022ULL);
  const auto v = draws.template host<double>();
  double mean = 0.0;
  for (double a : v) mean += a;
  mean /= static_cast<double>(v.size());
  double var = 0.0, below = 0.0;
  for (double a : v) {
    var += (a - mean) * (a - mean);
    if (a <= threshold) below += 1.0;
  }
  var /= static_cast<double>(v.size());
  const double p = d.cdf(pluss::make_series(std::vector<double>{threshold}, {1}).to(pluss::ScalarType::Float64)).template item<double>();
  std::cout << std::left << std::setw(20) << name << std::right << std::setw(10) << mean << std::setw(10) << d.cumulant(1).template item<double>() << std::setw(10) << var << std::setw(10)
            << d.cumulant(2).template item<double>() << std::setw(12) << below / static_cast<double>(v.size()) << std::setw(10) << p << "\n";
}
int main() {
  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(20) << "200000 draws" << std::right << std::setw(10) << "mean" << std::setw(10) << "exact" << std::setw(10) << "variance" << std::setw(10) << "exact"
            << std::setw(12) << "P(x<=t)" << std::setw(10) << "cdf(t)" << "\n";
  check("Normal(0.5, 2)", distr::Normal{0.5, 2.0}, 1.0);
  check("Gamma(2.5, 1.3)", distr::Gamma{2.5, 1.3, 0.0}, 3.0);
  check("Beta(2.5, 4)", distr::Beta{0.0, 1.0, 2.5, 4.0}, 0.4);
  check("Poisson(3.2)", distr::Poisson{3.2}, 3.0);
  check("Weibull(1.7, 2.1)", distr::Weibull{1.7, 2.1}, 2.0);

  // Reproducibility, shapes and dtypes.
  const distr::Normal n{0.0, 1.0};
  const Series a = n.sample({4}, {}, 7ULL), b = n.sample({4}, {}, 7ULL), c = n.sample({4}, {}, 8ULL);
  std::cout << "\nsame seed, same draws:      " << ((a - b).abs().max().item<double>() == 0.0 ? "yes" : "no") << "\n";
  std::cout << "different seed, different:  " << ((a - c).abs().max().item<double>() > 0.0 ? "yes" : "no") << "\n";
  const Series grid = n.sample({2, 3}, {}, 1ULL);
  std::cout << "sample({2, 3}) has shape    [" << grid.sizes()[0] << ", " << grid.sizes()[1] << "]\n";
  const Series like = pluss::series::zeros({5, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float32));
  const Series shaped = n.sample_like(like, 3ULL);
  std::cout << "sample_like a 5 x 2 float32 array: shape [" << shaped.sizes()[0] << ", " << shaped.sizes()[1] << "], dtype float32: " << (shaped.scalar_type() == pluss::ScalarType::Float32 ? "yes" : "no") << "\n";
}
