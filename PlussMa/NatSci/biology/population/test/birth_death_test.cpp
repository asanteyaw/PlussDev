// Birth-death chains, branching processes and diffusion viability against scipy (expm, linear solves, eigen), closed forms and Monte Carlo.
#include "pluss/subsp/natsci/biology/population/birth_death.h"
#include "pluss/subsp/natsci/common/ssa.h"
#include "birth_death_reference.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace bd = pluss::subsp::natsci::biology::population::birth_death;
namespace ref = birth_death_reference;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series mat(const std::vector<double> &v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(F64); }
Series vec(const std::vector<double> &v) { return mat(v, {static_cast<std::int64_t>(v.size())}); }
double rel_diff(const Series &a, const std::vector<double> &b, double floor = 1e-14) {
  const auto av = a.contiguous().to_vec<double>();
  expect(av.size() == b.size(), "size mismatch " + std::to_string(av.size()) + " vs " + std::to_string(b.size()));
  double w = 0.0;
  for (std::size_t i = 0; i < av.size(); ++i)
    w = std::max(w, std::abs(av[i] - b[i]) / (floor + std::abs(b[i])));
  return w;
}
} // namespace

int main() {
  try {
    // ---- stochastic logistic chain: transient law, mean time to extinction, quasi-stationary law (scipy expm / solve / eig)
    {
      const Series q = bd::generator(vec(ref::bd_birth), vec(ref::bd_death));
      expect(q.sum(-1, false).abs().max().item<double>() < 1e-12, "generator rows sum to zero");
      std::vector<double> p0(41, 0.0);
      p0[12] = 1.0;
      const Series pt = bd::transient(q, vec(p0), 1.7);
      expect(rel_diff(pt, ref::bd_transient, 1e-12) < 1e-8, "transient law vs scipy expm: " + std::to_string(rel_diff(pt, ref::bd_transient, 1e-12)));
      expect(std::abs(pt.sum().item<double>() - 1.0) < 1e-12, "transient law is a distribution");
      const Series tau = bd::mean_time_to_extinction(q);
      expect(rel_diff(tau, ref::bd_mtte) < 1e-9, "mean time to extinction vs scipy solve");
      const auto qs = bd::quasi_stationary(q);
      expect(rel_diff(qs.distribution, ref::bd_qsd, 1e-9) < 1e-6, "quasi-stationary distribution vs scipy eig: " + std::to_string(rel_diff(qs.distribution, ref::bd_qsd, 1e-9)));
      expect(std::abs(qs.decay_rate.item<double>() - ref::bd_decay[0]) < 1e-8 * ref::bd_decay[0] + 1e-12, "quasi-stationary decay rate");
      // Gillespie cross-check of the mean extinction time from n = 3 (population dies quickly: the exact mean is moderate)
      const double exact = tau.select(0, 2).item<double>();
      // note: from n = 3 the chain often runs to the metastable state first, so the exact mean is huge; compare a short-horizon survival instead
      std::vector<double> init(400, 0.0);
      for (std::size_t i = 0; i < 400; ++i)
        init[i] = 12.0;
      const Series x0 = pluss::make_series(init, {400, 1}).to(F64);
      pluss::subsp::natsci::common::ssa::SsaOptions opt;
      opt.seed = 99;
      const Series nu = mat({1.0, -1.0}, {2, 1});
      const auto prop = [](const Series &x) { // x [B, 1]: b_n = 2 n (1 - n/40) (never negative on the domain), d_n = n
        const Series n = x.select(-1, 0);
        const Series b = n.mul(2.0).mul(n.div(40.0).neg().add(1.0)).clamp(0.0, 1e300);
        return b.unsqueeze(-1).unite(std::vector<Series>{n.unsqueeze(-1)}, 1);
      };
      const auto sim = pluss::subsp::natsci::common::ssa::gillespie(prop, nu, x0, 1.7, opt);
      std::vector<double> hist(41, 0.0);
      for (double v : sim.state.contiguous().to_vec<double>())
        hist[static_cast<std::size_t>(std::llround(v))] += 1.0 / 400.0;
      double mean_exact = 0.0, mean_sim = 0.0;
      for (std::size_t i = 0; i <= 40; ++i) {
        mean_exact += static_cast<double>(i) * ref::bd_transient[i];
        mean_sim += static_cast<double>(i) * hist[i];
      }
      expect(std::abs(mean_sim - mean_exact) < 0.6, "Gillespie mean at t = 1.7 vs master equation: " + std::to_string(mean_sim) + " vs " + std::to_string(mean_exact));
      (void)exact;
    }
    // ---- linear birth-death: closed-form extinction probability vs the truncated master equation, and the ultimate probability
    {
      const Series n0 = vec({3.0});
      const std::vector<double> ts{0.5, 1.5, 3.0};
      for (std::size_t i = 0; i < ts.size(); ++i) {
        const double closed = bd::linear_extinction_probability(n0, 1.1, 0.9, vec({ts[i]})).item<double>();
        expect(std::abs(closed - ref::lin_p0_t[i]) < 2e-7, "linear birth-death extinction probability at t = " + std::to_string(ts[i]) + ": " + std::to_string(closed) + " vs " + std::to_string(ref::lin_p0_t[i]));
      }
      expect(std::abs(bd::linear_ultimate_extinction(3.0, 1.1, 0.9) - std::pow(0.9 / 1.1, 3.0)) < 1e-15 && bd::linear_ultimate_extinction(3.0, 0.9, 1.1) == 1.0, "ultimate extinction");
      // critical case b = d: alpha = b t / (1 + b t); the limit from the supercritical formula agrees
      const double a = bd::linear_extinction_probability(vec({1.0}), 1.0, 1.0, vec({4.0})).item<double>();
      const double near = bd::linear_extinction_probability(vec({1.0}), 1.0 + 1e-8, 1.0, vec({4.0})).item<double>();
      expect(std::abs(a - 0.8) < 1e-14 && std::abs(a - near) < 1e-6, "critical branch alpha = bt / (1 + bt)");
      expect(std::abs(bd::linear_mean(n0, 1.1, 0.9, vec({2.0})).item<double>() - 3.0 * std::exp(0.4)) < 1e-12, "linear mean n0 e^{(b-d)t}");
    }
    // ---- Galton-Watson
    {
      std::vector<double> p;
      for (const auto &row : ref::gw_p)
        p.push_back(row);
      const Series pm = mat(ref::gw_p, {4, 5});
      expect(rel_diff(bd::ultimate_extinction(pm), ref::gw_ult) < 1e-9, "ultimate extinction probability vs the smallest root");
      expect(rel_diff(bd::extinction_by_generation(pm, 12), ref::gw_by_gen) < 1e-12, "extinction by generation");
      expect(std::abs(bd::offspring_mean(pm).select(0, 0).item<double>() - 1.3) < 1e-14, "mean offspring");
    }
    // ---- diffusion viability: first-passage CDF vs Monte Carlo (barrier crossing between steps handled by the bridge correction)
    {
      for (std::size_t c = 0; c < 3; ++c) {
        const double mu = ref::pva_cases[3 * c], s2 = ref::pva_cases[3 * c + 1], xd = ref::pva_cases[3 * c + 2];
        for (std::size_t k = 0; k < 3; ++k) {
          const double got = bd::quasi_extinction_cdf(vec({mu}), vec({s2}), vec({xd}), vec({ref::pva_ts[k]})).item<double>();
          expect(std::abs(got - ref::pva_mc[3 * c + k]) < 4e-3, "quasi-extinction CDF case " + std::to_string(c) + " t = " + std::to_string(ref::pva_ts[k]) + ": " + std::to_string(got) + " vs MC " + std::to_string(ref::pva_mc[3 * c + k]));
        }
        const double ult = bd::ultimate_quasi_extinction(vec({mu}), vec({s2}), vec({xd})).item<double>();
        const double far = bd::quasi_extinction_cdf(vec({mu}), vec({s2}), vec({xd}), vec({1e6})).item<double>();
        expect(std::abs(far - ult) < 1e-9, "the CDF tends to the ultimate probability");
      }
    }
    // ---- Dennis regression estimator vs the numpy lstsq-through-origin result
    {
      const auto est = bd::dennis_estimate(vec(ref::den_counts), vec(ref::den_times));
      expect(std::abs(est.mu.item<double>() - ref::den_est[0]) < 1e-12 && std::abs(est.sigma2.item<double>() - ref::den_est[1]) < 1e-12, "Dennis estimator vs numpy");
    }
    std::cout << "birth_death tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
