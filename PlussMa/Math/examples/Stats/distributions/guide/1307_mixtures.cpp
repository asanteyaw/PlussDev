// Mixtures of normals. `GaussianMixture{weights, means, stddevs}` (each a Series with one entry per component) is the distribution of a draw from component j with probability weights[j].
// It has the usual `pdf`, `logpdf`, `cdf`, `sample`, and `MLE(x)` fits the weights, means and standard deviations by the EM algorithm, started from the values stored in the object (the
// number of components is its length) and run until the log-likelihood stops changing, so the result does not depend on the starting values unless they sit in different local maxima. The fit has
// `weights`, `means`, `stddevs`, `loglik` and the statistics of every fit: `std_err`, `vcov` and `confint` are for the natural parameters in the order weights (k), means (k), standard deviations (k)
// (by the delta method: the weights sum to one, so their covariance matrix is singular), while `score` and `observed_inf` use the unconstrained coordinates (logits of the first k - 1
// weights, means, logarithms of the standard deviations).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(12) << label << std::right;
  for (double v : s.reshape({s.numel()}).host<double>()) std::cout << std::setw(10) << v;
  std::cout << "\n";
}
int main() {
  const auto cfg = pluss::Config{}.with_dtype(f64);
  const distr::GaussianMixture truth{vec({0.3, 0.7}), vec({-2.0, 1.5}), vec({0.8, 1.2})};
  const Series x = truth.sample({600}, cfg, 77ULL);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "true            weights 0.3 0.7   means -2.0 1.5   standard deviations 0.8 1.2\n\n";

  // Two very different starting points reach the same maximum.
  for (const auto& start : std::vector<std::vector<double>>{{-1.0, 1.0}, {-3.0, 3.0}}) {
    const auto fit = distr::GaussianMixture{vec({0.5, 0.5}), vec(start), vec({1.0, 1.0})}.MLE(x);
    std::cout << "start at means (" << start[0] << ", " << start[1] << ")\n";
    row("  weights", fit.weights);
    row("  means", fit.means);
    row("  stddevs", fit.stddevs);
    std::cout << "  log-likelihood " << fit.loglik(x).item<double>() << "\n";
  }

  const auto fit = distr::GaussianMixture{vec({0.5, 0.5}), vec({-1.0, 1.0}), vec({1.0, 1.0})}.MLE(x);
  std::cout << "\nstandard errors of (w1, w2, m1, m2, s1, s2)\n";
  row("  stderr", fit.std_err(x));
  const auto ci = fit.confint(x).host<double>();
  std::cout << "  95% interval of the first mean   " << ci[4] << "  to  " << ci[5] << "\n";

  // One normal against two: the likelihood gain pays for the three extra parameters.
  const auto one = distr::Normal{}.MLE(x);
  std::cout << "\nlog-likelihood: one normal " << std::setprecision(2) << one.loglik(x).item<double>() << ", two components " << fit.loglik(x).item<double>() << "   (Akaike 2k - 2 loglik: "
            << 2 * 2 - 2 * one.loglik(x).item<double>() << " against " << 2 * 5 - 2 * fit.loglik(x).item<double>() << ")\n";
}
