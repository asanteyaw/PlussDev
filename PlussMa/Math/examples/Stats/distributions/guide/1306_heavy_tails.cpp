// Heavy tails. The normal inverse Gaussian (`Nig{alpha, beta, location, scale}`) and the generalised hyperbolic (`Gh{lambda, alpha, beta, location, scale}`) families describe skewed,
// heavy-tailed data, such as financial returns: `alpha` sets the tail weight (the larger, the lighter), `beta` the skewness (|beta| < alpha), `location` the centre and `scale` the spread;
// the Gh adds the shape `lambda` (lambda = -1/2 is the NIG). `Nig::MLE` fits all four parameters by maximising the exact log-likelihood (the density involves the modified Bessel function of
// the second kind, evaluated to full precision). A normal and an NIG are fitted to a sample from an NIG and compared by log-likelihood, Akaike's criterion (2 k - 2 loglik) and the
// probability of an extreme fall: the normal, with the same mean and variance, finds such falls far less likely than they are.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace distr = pluss::subsp::stats::distr;
using pluss::Series;

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = distr::Nig{1.5, -0.4, 0.1, 0.8}.sample({2000}, cfg, 20261022ULL);

  const auto normal = distr::Normal{}.MLE(x);
  const auto nig = distr::Nig{}.MLE(x);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "true NIG(1.5, -0.4, 0.1, 0.8)\n";
  std::cout << "fitted NIG   alpha " << nig.alpha << "  beta " << nig.beta << "  location " << nig.location << "  scale " << nig.scale << "\n";
  const auto se = nig.stderr(x).to_vec<double>();
  std::cout << "std errors        " << std::setw(6) << se[0] << "       " << std::setw(6) << se[1] << "            " << std::setw(6) << se[2] << "         " << std::setw(6) << se[3] << "\n\n";

  const double ll_n = normal.loglik(x).item<double>(), ll_g = nig.loglik(x).item<double>();
  std::cout << std::setprecision(2) << std::setw(10) << "model" << std::setw(14) << "loglik" << std::setw(10) << "AIC" << "\n";
  std::cout << std::setw(10) << "normal" << std::setw(14) << ll_n << std::setw(10) << 2 * 2 - 2 * ll_n << "\n";
  std::cout << std::setw(10) << "NIG" << std::setw(14) << ll_g << std::setw(10) << 2 * 4 - 2 * ll_g << "\n\n";

  // Probability of a fall below -3 (about three standard deviations): empirical, normal and NIG.
  const double cut = -3.0;
  const auto v = x.to_vec<double>();
  double below = 0.0;
  for (double a : v) if (a < cut) below += 1.0;
  const Series t = pluss::make_series(std::vector<double>{cut}, {1}).to(pluss::ScalarType::Float64);
  std::cout << std::setprecision(5) << "P(x < " << cut << ")   sample " << below / static_cast<double>(v.size()) << "   normal fit " << normal.model().cdf(t).item<double>() << "   NIG fit "
            << nig.model().cdf(t).item<double>() << "\n";
}
