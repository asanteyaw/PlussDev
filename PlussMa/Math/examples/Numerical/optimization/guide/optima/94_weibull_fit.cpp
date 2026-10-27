// Maximum likelihood, with an answer you can check. Forty lifetimes are placed at the evenly spaced quantiles of a
// Weibull distribution with shape 2.5 and scale 100, so a good fit should come close to those values. The unknowns are the
// shape k and the scale s, both positive (bounds). The objective is the negative log-likelihood. A check that needs no
// reference answer follows every fit: at a maximum of the likelihood the scale satisfies s^k = mean(x^k) exactly. The
// fit is run from four starting points, the last of them deliberately poor, to show what the exit flag does and does not
// promise.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> ks(double k, double s) { return optim::pack_scalars({{"k", scalar(k)}, {"s", scalar(s)}}); }

std::vector<double> lifetimes() {
  std::vector<double> x;
  for (int i = 0; i < 40; ++i) x.push_back(100.0 * std::pow(-std::log(1.0 - (i + 0.5) / 40.0), 1.0 / 2.5));
  return x;
}

int main() {
  const auto x = lifetimes();
  std::vector<double> log_x;
  for (double v : x) log_x.push_back(std::log(v));
  const auto log_data = pluss::make_series(log_x, {40}).to(f64);

  // negative log-likelihood = -sum[ ln k - k ln s + (k - 1) ln x - (x / s)^k ]
  auto nll = [&](const Pack& p) {
    const pluss::Series constants = log_data;
    const pluss::Series k = p.at("k"), log_s = p.at("s").log();
    const pluss::Series term = k.log() - k * log_s + (k - 1.0) * constants - (k * (constants - log_s)).exp();
    return -1.0 * term.sum();
  };
  const auto fit = optim::constraints().lbs(ks(0.1, 1.0)).ubs(ks(20.0, 1000.0));

  const auto mean_power = [&](double k) {
    double total = 0.0;
    for (double v : x) total += std::pow(v, k);
    return total / static_cast<double>(x.size());
  };

  std::cout << std::fixed << std::setprecision(4);
  std::cout << std::left << std::setw(14) << "start" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(20) << "fitted k, s"
            << std::setw(14) << "optimality" << "s^k / mean(x^k)\n";
  const std::pair<double, double> starts[] = {{1.0, 50.0}, {5.0, 200.0}, {2.0, 80.0}, {10.0, 20.0}};
  for (const auto& [k0, s0] : starts) {
    const auto r = optim::optima(nll, ks(k0, s0), {}, fit);
    const double k = as_f64(r.params.at("k")), s = as_f64(r.params.at("s"));
    std::ostringstream label, fitted, optimality;
    label << "(" << k0 << ", " << s0 << ")";
    fitted << std::fixed << std::setprecision(4) << k << ", " << s;
    optimality << std::scientific << std::setprecision(1) << r.output.first_order_optimality;
    std::cout << std::left << std::setw(14) << label.str() << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration
              << std::setw(20) << fitted.str() << std::setw(14) << optimality.str() << std::fixed << std::setprecision(4)
              << std::pow(s, k) / mean_power(k) << "\n";
  }
  return 0;
}
