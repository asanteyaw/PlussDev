// Environmental and demographic noise: lognormal law of density-independent growth, the gamma stationary law of the stochastic logistic, bridge-corrected first
// passage against the exact Brownian result, and Feller's extinction probability against the exact linear birth-death master equation.
#include "pluss/subsp/natsci/biology/population/birth_death.h"
#include "pluss/subsp/natsci/biology/population/stochastic_growth.h"

#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

namespace sg = pluss::subsp::natsci::biology::population::stochastic_growth;
namespace bd = pluss::subsp::natsci::biology::population::birth_death;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
Series lanes(std::int64_t n, double value) { return pluss::series::zeros({n}, pluss::Config{}.with_dtype(F64)).add(value); }
} // namespace

int main() {
  try {
    // ---- geometric Brownian growth: ln N_T ~ Normal(ln N0 + (r - s^2/2) T, s^2 T); moments of N_T from the lognormal law
    {
      constexpr double r = 0.4, s = 0.5, t = 2.0, n0 = 10.0;
      constexpr std::int64_t m = 40000;
      const auto drift = [](double, const Series &x, const Series &) { return x.mul(0.0).add(r - 0.5 * s * s); };
      const auto diffusion = [](double, const Series &x, const Series &) { return x.mul(0.0).add(s); };
      const auto p = sg::simulate(drift, diffusion, lanes(m, std::log(n0)), t, 4, 8, 11);
      const Series x = p.x.select(0, 4);
      const double mean = x.mean().item<double>(), var = x.sub(mean).square().mean().item<double>();
      expect(std::abs(mean - (std::log(n0) + sg::stochastic_growth_rate(r, s) * t)) < 4.0 * s * std::sqrt(t) / std::sqrt(static_cast<double>(m)), "mean of ln N_T");
      expect(std::abs(var / (s * s * t) - 1.0) < 4.0 * std::sqrt(2.0 / static_cast<double>(m)), "variance of ln N_T");
      const double mean_n = x.exp().mean().host<double>().front(), exact = sg::lognormal_mean(vec({n0}), r, t).host<double>().front();
      expect(std::abs(mean_n / exact - 1.0) < 0.02, "E[N_T] = N0 e^{rt}: " + std::to_string(mean_n) + " vs " + std::to_string(exact));
      expect(sg::lognormal_variance(vec({n0}), r, s, t).host<double>().front() > 0.0 && sg::stochastic_growth_rate(0.1, 0.5) < 0.0, "growth can be negative on average while E[N] grows (Lewontin-Cohen)");
    }
    // ---- stochastic logistic: stationary Gamma(2r/s^2 - 1, 2r/(K s^2)) mean and variance (z-test, 30000 independent lanes)
    {
      const sg::Logistic model{.r = 1.0, .k = 100.0, .s = 0.5};
      expect(model.persistent() && std::abs(model.mean() - 100.0 * (1.0 - 0.125)) < 1e-12, "closed-form stationary mean K (1 - s^2 / 2r)");
      constexpr std::int64_t m = 30000;
      const auto p = sg::simulate(model.drift(), model.diffusion(), lanes(m, std::log(80.0)), 25.0, 5, 500, 5);
      const Series n = p.x.select(0, 5).exp();
      const double mean = n.mean().item<double>(), var = n.sub(mean).square().mean().item<double>();
      const double se_mean = std::sqrt(model.variance() / static_cast<double>(m));
      expect(std::abs(mean - model.mean()) < 5.0 * se_mean + 0.01 * model.mean() * 0.1, "stationary mean " + std::to_string(mean) + " vs " + std::to_string(model.mean()));
      expect(std::abs(var / model.variance() - 1.0) < 0.05, "stationary variance " + std::to_string(var) + " vs " + std::to_string(model.variance()));
      // a larger noise destroys persistence
      expect(!sg::Logistic{.r = 0.2, .k = 50.0, .s = 0.7}.persistent(), "s^2 >= 2r: no stationary law");
    }
    // ---- first passage of ln N below the quasi-extinction barrier: bridge-corrected estimate vs the exact Brownian first-passage CDF
    {
      constexpr double r = 0.06, s = 0.3, mu = r - 0.5 * s * s, n0 = 100.0, nc = 20.0, horizon = 30.0;
      const auto drift = [](double, const Series &x, const Series &) { return x.mul(0.0).add(mu); };
      const auto diffusion = [](double, const Series &x, const Series &) { return x.mul(0.0).add(s); };
      constexpr std::int64_t m = 6000, blocks = 30;
      const auto p = sg::simulate(drift, diffusion, lanes(m, std::log(n0)), horizon, blocks, 2, 21);
      const double estimate = sg::crossing_probability(p.x, std::log(nc), s, p.block_time).mean().item<double>();
      const double exact = bd::quasi_extinction_cdf(vec({mu}), vec({s * s}), vec({std::log(n0 / nc)}), vec({horizon})).host<double>().front();
      expect(std::abs(estimate - exact) < 0.01, "bridge-corrected first passage " + std::to_string(estimate) + " vs exact " + std::to_string(exact));
      // counting only sampled crossings underestimates; the corrected estimator is closer than the naive one
      const double naive = p.x.le(std::log(nc)).to(F64).amax(0, false).mean().item<double>();
      expect(naive <= estimate + 1e-12 && std::abs(estimate - exact) < std::abs(naive - exact), "the bridge correction removes the coarse-sampling bias");
    }
    // ---- Feller diffusion extinction probability vs the exact linear birth-death master equation at increasing scale
    {
      constexpr double r = 0.5, t = 3.0;
      double previous = 1.0; // the diffusion limit is an expansion in r / (b + d): the gap shrinks monotonically in this regime
      for (const double scale : {16.0, 64.0, 256.0, 1024.0}) {
        const double birth = 0.5 * (scale + r), death = 0.5 * (scale - r), sigma2 = birth + death; // per-capita rates with b - d = r, b + d = sigma_d^2
        const double n0 = 3.0;
        const double exact = bd::linear_extinction_probability(vec({n0}), birth, death, vec({t})).host<double>().front();
        const double feller = sg::feller_extinction_probability(vec({n0}), r, sigma2, t).host<double>().front();
        const double gap = std::abs(exact - feller);
        expect(gap < previous + 1e-12, "the diffusion approximation tightens as the per-individual demographic variance b + d grows relative to r: scale " + std::to_string(scale) + ", gap " + std::to_string(gap));
        previous = gap;
      }
      expect(previous < 1e-6, "agreement to 1e-6 at b + d = 1024 (r = 0.5)");
    }
    std::cout << "stochastic_growth tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
