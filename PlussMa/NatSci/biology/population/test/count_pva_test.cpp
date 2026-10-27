// Count-based PVA with observation error: Kalman likelihood, maximum likelihood fit, smoother, standard errors and extinction risk vs an independent numpy/scipy
// implementation (scalar Kalman + RTS, L-BFGS-B/Nelder-Mead, finite-difference Hessian, quad), plus the eta^2 -> 0 Dennis limit and a closed-form ultimate risk.
#include "pluss/subsp/natsci/biology/population/count_pva.h"
#include "count_pva_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pva = pluss::subsp::natsci::biology::population::count_pva;
namespace bd = pluss::subsp::natsci::biology::population::birth_death;
namespace ref = count_pva_reference;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::series::make(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double rel(double a, double b) { return std::abs(a / b - 1.0); }
} // namespace

int main() {
  try {
    const Series counts = vec(ref::counts);
    for (std::size_t i = 0; i < 3; ++i) {
      const double ll = pva::loglik(counts, {ref::fixed[3 * i], ref::fixed[3 * i + 1], ref::fixed[3 * i + 2]});
      expect(std::abs(ll - ref::fixed_ll[i]) < 1e-9 * (1.0 + std::abs(ref::fixed_ll[i])), "Kalman log-likelihood vs numpy: " + std::to_string(ll) + " vs " + std::to_string(ref::fixed_ll[i]));
    }
    const auto f = pva::fit(counts);
    expect(f.loglik >= ref::mle_ll[0] - 1e-7, "ML fit reaches the reference likelihood: " + std::to_string(f.loglik) + " vs " + std::to_string(ref::mle_ll[0]));
    expect(std::abs(f.parameters.mu - ref::mle[0]) < 2e-4 && rel(f.parameters.sigma2, ref::mle[1]) < 2e-3 && rel(f.parameters.eta2, ref::mle[2]) < 2e-3, "ML estimates vs scipy");
    expect(rel(f.mu_se, ref::se[0]) < 5e-3 && rel(f.sigma2_se, ref::se[1]) < 1e-2 && rel(f.eta2_se, ref::se[2]) < 1e-2, "standard errors vs finite-difference Hessian");
    expect(std::abs(f.filtered_mean - ref::filt[0]) < 2e-4 && rel(f.filtered_var, ref::filt[1]) < 2e-3, "filtered state at the last census");
    const auto sm = f.smoothed_mean.host<double>(), ss = f.smoothed_sd.host<double>();
    expect(sm.size() == ref::smoothed_mean.size(), "smoother length");
    double worst = 0.0, worst_sd = 0.0;
    for (std::size_t i = 0; i < sm.size(); ++i) {
      worst = std::max(worst, std::abs(sm[i] - ref::smoothed_mean[i]));
      worst_sd = std::max(worst_sd, std::abs(ss[i] / ref::smoothed_sd[i] - 1.0));
    }
    expect(worst < 5e-4 && worst_sd < 5e-3, "smoother vs RTS: " + std::to_string(worst) + " / " + std::to_string(worst_sd));
    // extinction risk with the reference parameters (isolates the integration from the optimiser)
    {
      pva::Fit g = f;
      g.parameters = {ref::mle[0], ref::mle[1], ref::mle[2]};
      g.filtered_mean = ref::filt[0];
      g.filtered_var = ref::filt[1];
      const auto r = pva::extinction_risk(g, ref::threshold[0], vec(ref::horizons)).host<double>();
      for (std::size_t i = 0; i < r.size(); ++i)
        expect(std::abs(r[i] - ref::risk[i]) < 1e-9, "quasi-extinction risk vs quad at horizon " + std::to_string(ref::horizons[i]));
      expect(pva::ultimate_extinction_risk(g, ref::threshold[0]) == ref::ultimate[0], "non-positive drift: certain ultimate extinction");
      // positive drift: closed Gaussian integral  Phi(-m/sqrt P) + exp(-2 mu m/s2 + 2 mu^2 P/s2^2) Phi((m - 2 mu P/s2)/sqrt P)
      g.parameters = {0.03, 0.02, 0.01};
      g.filtered_mean = std::log(400.0);
      g.filtered_var = 0.04;
      const double m = g.filtered_mean - std::log(50.0), P = g.filtered_var, s2 = g.parameters.sigma2, mu = g.parameters.mu;
      auto Phi = [](double z) { return 0.5 * std::erfc(-z / std::sqrt(2.0)); };
      const double exact = Phi(-m / std::sqrt(P)) + std::exp(-2.0 * mu * m / s2 + 2.0 * mu * mu * P / (s2 * s2)) * Phi((m - 2.0 * mu * P / s2) / std::sqrt(P));
      expect(std::abs(pva::ultimate_extinction_risk(g, 50.0) - exact) < 1e-10, "ultimate risk vs closed form");
      const auto long_run = pva::extinction_risk(g, 50.0, vec({1e6})).host<double>().front();
      expect(std::abs(long_run - exact) < 1e-8, "risk over a very long horizon approaches the ultimate risk");
    }
    // no observation error: ML reduces to the Dennis-Munholland-Scott estimator (sigma^2 with 1/n rather than 1/(n-1))
    {
      std::vector<double> n(30);
      double x = std::log(200.0);
      std::uint64_t s = 88172645463325252ULL;
      auto u = [&] { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (static_cast<double>(s >> 11) + 0.5) / 9007199254740992.0; };
      for (auto &v : n) {
        v = std::exp(x);
        x += 0.02 + 0.15 * std::sqrt(-2.0 * std::log(u())) * std::cos(2.0 * M_PI * u());
      }
      const Series c = vec(n);
      std::vector<double> t(n.size());
      for (std::size_t i = 0; i < t.size(); ++i)
        t[i] = static_cast<double>(i);
      const auto d = bd::dennis_estimate(c, vec(t));
      const auto g = pva::fit(c);
      const double q = static_cast<double>(n.size() - 1);
      expect(std::abs(g.parameters.mu - d.mu.item<double>()) < 1e-3 && g.parameters.eta2 < 5e-3, "noise-free series: mu matches Dennis, eta^2 ~ 0");
      expect(rel(g.parameters.sigma2, d.sigma2.item<double>() * (q - 1.0) / q) < 0.08, "noise-free series: sigma^2 near the Dennis value");
    }
    // grid placement of irregular census times
    {
      const auto g = pva::to_grid(vec({1990.0, 1991.0, 1994.0, 1995.0}), vec({10.0, 12.0, 9.0, 11.0}), 1.0);
      const auto v = g.counts.host<double>();
      expect(v.size() == 6 && std::isnan(v[2]) && std::isnan(v[3]) && v[4] == 9.0 && v[5] == 11.0, "to_grid places counts and leaves gaps missing");
    }
    std::cout << "count_pva tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
