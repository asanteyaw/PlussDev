// Families without a closed-form estimate, and fixed parameters. The gamma shape, the beta parameters and the negative binomial r solve likelihood equations with no closed form: they are
// found by Newton's method to full precision, and the standard errors come from the full information matrix, in which the parameters are correlated. Gamma, Beta and Binomial hold some
// parameters fixed because the likelihood is not regular in them: the gamma `location`, the beta `low` and `high`, and the binomial `count` are taken from the object `MLE` is called on and
// reported without uncertainty (standard error 0, interval of zero width, zero rows and columns in `vcov`). The shape and scale of a gamma are strongly correlated, so the interval of
// each alone is not the whole story: the correlation of the estimates is printed.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

void row(const char* label, const Series& s) {
  std::cout << std::left << std::setw(16) << label << std::right;
  for (double v : s.reshape({s.numel()}).to_vec<double>()) std::cout << std::setw(11) << v;
  std::cout << "\n";
}
int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  std::cout << std::fixed << std::setprecision(4);

  const Series xg = distr::Gamma{2.5, 1.3, 0.0}.sample({500}, cfg, 20261022ULL);
  const auto gf = distr::Gamma{1.0, 1.0, 0.0}.MLE(xg);
  std::cout << "Gamma sample (true shape 2.5, scale 1.3, location 0)\n";
  std::cout << "  shape " << gf.shape << "   scale " << gf.scale << "   location (fixed) " << gf.location << "\n";
  row("  stderr", gf.stderr(xg));
  row("  95% intervals", gf.confint(xg));
  const auto v = gf.vcov(xg).to_vec<double>();
  std::cout << "  correlation of shape and scale " << v[1] / std::sqrt(v[0] * v[4]) << "   (vcov is 3 x 3, the location row and column are zero)\n";
  std::cout << "  log-likelihood " << gf.loglik(xg).item<double>() << "\n";

  const Series xb = distr::Beta{0.0, 1.0, 2.5, 4.0}.sample({500}, cfg, 7ULL);
  const auto bf = distr::Beta{0.0, 1.0, 1.0, 1.0}.MLE(xb);
  std::cout << "\nBeta sample on [0, 1] (true alpha 2.5, beta 4)\n  alpha " << bf.alpha << "   beta " << bf.beta << "   (low " << bf.low << ", high " << bf.high << " fixed)\n";
  row("  stderr", bf.stderr(xb));
  // The same data on [-1, 3]: telling the object the support is how the family is fitted there.
  std::vector<double> shifted;
  for (double a : xb.to_vec<double>()) shifted.push_back(-1.0 + 4.0 * a);
  const auto bf2 = distr::Beta{-1.0, 3.0, 1.0, 1.0}.MLE(pluss::make_series(shifted, {500}).to(pluss::ScalarType::Float64));
  std::cout << "  the same data moved to [-1, 3]: alpha " << bf2.alpha << "   beta " << bf2.beta << "\n";

  const Series xn = distr::NegativeBinomial{3.5, 0.4}.sample({500}, cfg, 11ULL);
  const auto nf = distr::NegativeBinomial{1.0, 0.5}.MLE(xn);
  std::cout << "\nNegative binomial sample (true r 3.5, p 0.4)\n  r " << nf.r << "   p " << nf.p << "\n";
  row("  stderr", nf.stderr(xn));
  row("  95% intervals", nf.confint(xn));
  const auto nv = nf.vcov(xn).to_vec<double>();
  std::cout << "  correlation of r and p " << nv[1] / std::sqrt(nv[0] * nv[3]) << "\n";

  // Counts that are not over-dispersed (variance below the mean) have no finite maximum: r grows without bound (the Poisson limit); the fit reports a large r.
  const auto flat = distr::NegativeBinomial{1.0, 0.5}.MLE(distr::Binomial{10, 0.5}.sample({400}, cfg, 13ULL));
  std::cout << "\nunder-dispersed counts (a binomial sample): r = " << std::scientific << std::setprecision(1) << flat.r << "\n";
}
