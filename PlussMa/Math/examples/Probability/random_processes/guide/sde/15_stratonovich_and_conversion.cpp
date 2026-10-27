// The same random process can be written in two conventions. For geometric Brownian motion, dX = mu X dt +
// sigma X dW in Ito form, the Stratonovich form is dX = (mu - sigma^2/2) X dt + sigma X o dW: the drift moves by
// half of sigma sigma'. `strat_drift` and `ito_drift` convert a drift (elementwise, one scalar process per entry);
// `strat` applies the ORDINARY chain rule, with no second-order term. The check at the end: log X has the same
// law whichever route computes it.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double mu_c = 0.12, sg = 0.4;
  const pluss::Series x = pluss::make_series(std::vector<double>{0.5, 1.0, 2.0}, {3}).to(f64);
  const pluss::Series mu = x.mul(mu_c);
  const auto sigma_of = [sg](const pluss::Series& s) { return s.mul(sg); };  // sigma(x) = 0.4 x

  const pluss::Series mu_strat = x.calc.diff.strat_drift(mu, sigma_of);
  const pluss::Series back = x.calc.diff.ito_drift(mu_strat, sigma_of);
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "Ito drift            " << mu.host<double>()[0] << " " << mu.host<double>()[1] << " " << mu.host<double>()[2] << "\n";
  std::cout << "Stratonovich drift   " << mu_strat.host<double>()[0] << " " << mu_strat.host<double>()[1] << " " << mu_strat.host<double>()[2]
            << "   (mu x - sigma^2 x / 2: " << (mu_c - 0.5 * sg * sg) * 0.5 << ", " << (mu_c - 0.5 * sg * sg) * 1.0 << ", "
            << (mu_c - 0.5 * sg * sg) * 2.0 << ")\n";
  std::cout << "converted back       " << back.host<double>()[0] << " " << back.host<double>()[1] << " " << back.host<double>()[2] << "\n\n";

  const auto g = [](const pluss::Series& s) { return s.log(); };
  const auto ito_route = x.calc.diff.ito(g, mu, x.mul(sg));
  const auto strat_route = x.calc.diff.strat(g, mu_strat, x.mul(sg));
  std::cout << "d log X, Ito's lemma:           drift " << ito_route.drift.host<double>()[0] << ", diffusion "
            << ito_route.diffusion.host<double>()[0] << "\n";
  std::cout << "d log X, ordinary chain rule:   drift " << strat_route.drift.host<double>()[0] << ", diffusion "
            << strat_route.diffusion.host<double>()[0] << "\n";
  std::cout << "both equal mu - sigma^2/2 = " << mu_c - 0.5 * sg * sg << " and sigma = " << sg << "\n";
  return 0;
}
