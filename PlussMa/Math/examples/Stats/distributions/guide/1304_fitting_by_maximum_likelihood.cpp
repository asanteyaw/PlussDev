// Fitting a distribution to data by maximum likelihood. `dist.MLE(x)` on a distribution object returns a fit object of the same family: its fields are the estimated parameters
// (`mean`, `stddev` for the normal; `rate` for the exponential; `lambda` for the Poisson ...). The parameters of the object that `MLE` is called on are not used, except for those the
// family holds fixed (the location of a gamma, the bounds of a beta, the count of a binomial: see the next program). The fit has the methods `loglik(x)` (total log-likelihood, a one-element Series), `loglik_per_obs(x)`,
// `nll(x)`, `score(x)` (gradient of the log-likelihood, zero at an interior maximum), `observed_inf(x)` and `fisher_inf(x)` (information matrix), `vcov(x)` (its inverse: the
// estimated covariance of the estimates), `std_err(x)` and `confint(x, level)` (Wald interval, one row per parameter). Here a normal, an exponential and a Poisson model are fitted to
// samples from those families; the standard errors agree with the exact formulas sigma / sqrt(n), sigma / sqrt(2n), rate / sqrt(n) and sqrt(lambda / n).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right;
  for (double v : s.reshape({s.numel()}).host<double>()) std::cout << std::setw(11) << v;
  std::cout << "\n";
}
int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double n = 500.0;
  std::cout << std::fixed << std::setprecision(4);

  const Series xn = distr::Normal{1.5, 2.0}.sample({500}, cfg, 20261022ULL);
  const auto nf = distr::Normal{}.MLE(xn);
  std::cout << "Normal sample (true mean 1.5, sd 2.0)\n";
  std::cout << "  estimates     mean " << nf.mean << "   stddev " << nf.stddev << "\n";
  row("  stderr", nf.std_err(xn));
  std::cout << "  exact         " << std::setw(11) << nf.stddev / std::sqrt(n) << std::setw(11) << nf.stddev / std::sqrt(2 * n) << "\n";
  row("  95% interval", nf.confint(xn));
  row("  score", nf.score(xn));
  row("  vcov", nf.vcov(xn));
  std::cout << "  log-likelihood " << nf.loglik(xn).item<double>() << "   per observation " << nf.loglik(xn).item<double>() / n << "\n\n";

  const Series xe = distr::Exponential{0.7}.sample({500}, cfg, 3ULL);
  const auto ef = distr::Exponential{}.MLE(xe);
  std::cout << "Exponential sample (true rate 0.7)\n  rate " << ef.rate << "   stderr " << ef.std_err(xe).item<double>() << "   exact " << ef.rate / std::sqrt(n) << "\n";
  row("  95% interval", ef.confint(xe));

  const Series xp = distr::Poisson{3.2}.sample({500}, cfg, 4ULL);
  const auto pf = distr::Poisson{}.MLE(xp);
  std::cout << "\nPoisson sample (true lambda 3.2)\n  lambda " << pf.lambda << "   stderr " << pf.std_err(xp).item<double>() << "   exact " << std::sqrt(pf.lambda / n) << "\n";
  row("  95% interval", pf.confint(xp));
}
