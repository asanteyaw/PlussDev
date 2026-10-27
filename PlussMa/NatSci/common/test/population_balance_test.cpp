// Population balance: growth / nucleation finite volumes and fixed-pivot aggregation / breakage against closed-form solutions.
#include "pluss/subsp/natsci/common/population_balance.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace pb = pluss::subsp::natsci::common::population_balance;
namespace dyn = pluss::subsp::natsci::common::dynamics;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
double l2_error(const std::vector<double> &a, const std::vector<double> &b, double h) {
  double s = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i)
    s += (a[i] - b[i]) * (a[i] - b[i]) * h;
  return std::sqrt(s);
}
double gauss(double x, double m, double w) { return std::exp(-(x - m) * (x - m) / w); }
} // namespace

int main() {
  try {
    // ---- growth: pure translation n(L, t) = n0(L - G t); second-order convergence on a smooth profile
    {
      double prev = 0.0;
      for (const std::int64_t bins : {100, 200, 400}) {
        const auto g = pb::uniform_grid(0.0, 10.0, bins);
        std::vector<double> n0(static_cast<std::size_t>(bins)), ex(static_cast<std::size_t>(bins));
        for (std::int64_t i = 0; i < bins; ++i) {
          n0[static_cast<std::size_t>(i)] = gauss(g.centers[static_cast<std::size_t>(i)], 3.0, 0.5);
          ex[static_cast<std::size_t>(i)] = gauss(g.centers[static_cast<std::size_t>(i)], 5.0, 0.5);
        }
        const Series growth = vec(std::vector<double>(static_cast<std::size_t>(bins) + 1, 1.0));
        const auto rhs = [&](const Series &y) { return pb::growth_rhs(y, g, growth, 0.0); };
        const Series out = pb::ssprk3(rhs, vec(n0), 0.0, 2.0, bins * 2);
        const double err = l2_error(out.host<double>(), ex, g.width(0));
        if (prev > 0.0)
          expect(prev / err > 3.0, "growth scheme must converge at second order (error ratio " + std::to_string(prev / err) + ")");
        prev = err;
        expect(std::abs(pb::moment(out, g, 0).item<double>() - pb::moment(vec(n0), g, 0).item<double>()) < 1e-12, "growth conserves the number");
      }
      expect(prev < 5e-3, "growth error on the finest grid " + std::to_string(prev));
    }
    // ---- size-dependent growth G = a (1 + b L): characteristics, n(L, t) = n0(L0) exp(-a b t), L0 = ((1 + b L) exp(-a b t) - 1) / b
    {
      const double a = 0.4, b = 0.3, t = 3.0;
      const auto g = pb::uniform_grid(0.0, 40.0, 800);
      const std::int64_t bins = g.bins();
      std::vector<double> n0(static_cast<std::size_t>(bins)), ex(static_cast<std::size_t>(bins)), ge(static_cast<std::size_t>(bins) + 1);
      for (std::int64_t i = 0; i <= bins; ++i)
        ge[static_cast<std::size_t>(i)] = a * (1.0 + b * g.edges[static_cast<std::size_t>(i)]);
      for (std::int64_t i = 0; i < bins; ++i) {
        const double x = g.centers[static_cast<std::size_t>(i)];
        n0[static_cast<std::size_t>(i)] = gauss(x, 5.0, 2.0);
        const double l0 = ((1.0 + b * x) * std::exp(-a * b * t) - 1.0) / b;
        ex[static_cast<std::size_t>(i)] = gauss(l0, 5.0, 2.0) * std::exp(-a * b * t);
      }
      const Series growth = vec(ge);
      const auto rhs = [&](const Series &y) { return pb::growth_rhs(y, g, growth, 0.0); };
      const Series out = pb::ssprk3(rhs, vec(n0), 0.0, t, 3000);
      const double size_err = l2_error(out.host<double>(), ex, g.width(0));
      expect(size_err < 4e-3, "size-dependent growth vs characteristics, error " + std::to_string(size_err));
    }
    // ---- nucleation with growth: mu_0 = B0 t, mu_1 = G B0 t^2 / 2 (domain large enough that nothing leaves)
    {
      const auto g = pb::uniform_grid(0.0, 30.0, 600);
      const double growth_rate = 1.5, b0 = 2.0, t = 4.0;
      const Series growth = vec(std::vector<double>(static_cast<std::size_t>(g.bins()) + 1, growth_rate));
      const auto rhs = [&](const Series &y) { return pb::growth_rhs(y, g, growth, b0); };
      const Series out = pb::ssprk3(rhs, vec(std::vector<double>(static_cast<std::size_t>(g.bins()), 0.0)), 0.0, t, 800);
      // flux B0 = G n(0): n_nuclei number rate is B0, so mu_0 = B0 t and mu_1 = G B0 t^2 / 2 with the face flux = B0 (here B0 is a FLUX)
      expect(std::abs(pb::moment(out, g, 0).item<double>() - b0 * t) < 1e-9 * (b0 * t), "number of nuclei born = flux * time");
      expect(std::abs(pb::moment(out, g, 1).item<double>() / (growth_rate * b0 * t * t / 2.0) - 1.0) < 5e-3, "first moment of the grown nuclei");
    }
    // ---- MSMPR crystallizer steady state: n(L) = (B0 / G) exp(-L / (G tau)), total number B0 tau, mu_k = k! (B0/G) (G tau)^{k+1}
    {
      const auto g = pb::uniform_grid(0.0, 24.0, 960); // domain >> G tau = 2: the truncated tail of mu_2 is below 1e-4
      const double growth_rate = 0.5, flux = 1.0, tau = 4.0;
      const Series growth = vec(std::vector<double>(static_cast<std::size_t>(g.bins()) + 1, growth_rate));
      const auto rhs = [&](const Series &y) { return pb::growth_rhs(y, g, growth, flux).sub(y.div(tau)); };
      const Series out = pb::ssprk3(rhs, vec(std::vector<double>(static_cast<std::size_t>(g.bins()), 0.0)), 0.0, 12.0 * tau, 6000);
      const auto n = out.host<double>();
      double worst = 0.0;
      for (std::int64_t i = 0; i < g.bins(); ++i) {
        const double x = g.centers[static_cast<std::size_t>(i)];
        const double exact = flux / growth_rate * std::exp(-x / (growth_rate * tau));
        worst = std::max(worst, std::abs(n[static_cast<std::size_t>(i)] - exact) / (flux / growth_rate));
      }
      expect(worst < 5e-3, "MSMPR steady-state profile, worst deviation " + std::to_string(worst));
      expect(std::abs(pb::moment(out, g, 0).item<double>() / (flux * tau) - 1.0) < 3e-3, "MSMPR total number B0 tau");
      expect(std::abs(pb::moment(out, g, 2).item<double>() / (2.0 * flux / growth_rate * std::pow(growth_rate * tau, 3.0)) - 1.0) < 1e-2, "MSMPR second moment");
    }
    // ---- aggregation, discrete Smoluchowski (integer pivots) with closed forms
    {
      const std::int64_t bins = 200;
      const auto g = pb::integer_grid(bins);
      const Series like = vec({0.0});
      const auto op = pb::build_aggregation_operator(g, like);
      std::vector<double> n0(static_cast<std::size_t>(bins), 0.0);
      n0[0] = 1.0;
      // constant kernel K = 1: N_k = a^{k-1} / (1 + a)^{k+1}, a = t / 2
      {
        const Series kernel = vec(std::vector<double>(static_cast<std::size_t>(bins * bins), 1.0)).reshape({bins, bins});
        const dyn::RateFn rate = [&](const Series &y) { return pb::aggregation_rhs(y, op, kernel); };
        const double t = 4.0;
        const Series out = dyn::simulate(rate, vec(n0), 0.0, t, 400);
        const auto n = out.host<double>();
        const double a = t / 2.0;
        double worst = 0.0;
        for (std::int64_t k = 1; k <= 40; ++k)
          worst = std::max(worst, std::abs(n[static_cast<std::size_t>(k - 1)] - std::pow(a, k - 1) / std::pow(1.0 + a, k + 1)));
        expect(worst < 1e-7, "constant-kernel Smoluchowski vs the exact solution, worst " + std::to_string(worst));
        expect(std::abs(pb::pivot_moment(out, g, 1.0).item<double>() - 1.0) < 1e-6, "mass is conserved");
      }
      // additive kernel K = i + j: M0(t) = exp(-M1 t), mass constant
      {
        std::vector<double> k(static_cast<std::size_t>(bins * bins));
        for (std::int64_t i = 0; i < bins; ++i)
          for (std::int64_t j = 0; j < bins; ++j)
            k[static_cast<std::size_t>(i * bins + j)] = static_cast<double>(i + j + 2);
        const Series kernel = vec(k).reshape({bins, bins});
        const dyn::RateFn rate = [&](const Series &y) { return pb::aggregation_rhs(y, op, kernel); };
        const Series out = dyn::simulate(rate, vec(n0), 0.0, 1.0, 400);
        expect(std::abs(pb::pivot_moment(out, g, 0.0).item<double>() / std::exp(-1.0) - 1.0) < 1e-6, "additive kernel: M0 = exp(-M1 t)");
        expect(std::abs(pb::pivot_moment(out, g, 1.0).item<double>() - 1.0) < 1e-4, "additive kernel: mass conserved (truncation only)");
      }
      // product kernel K = i j, before gelation (t < 1): M0 = 1 - t/2, M2 = 1/(1 - t)
      {
        std::vector<double> k(static_cast<std::size_t>(bins * bins));
        for (std::int64_t i = 0; i < bins; ++i)
          for (std::int64_t j = 0; j < bins; ++j)
            k[static_cast<std::size_t>(i * bins + j)] = static_cast<double>((i + 1) * (j + 1));
        const Series kernel = vec(k).reshape({bins, bins});
        const dyn::RateFn rate = [&](const Series &y) { return pb::aggregation_rhs(y, op, kernel); };
        const double t = 0.6;
        const Series out = dyn::simulate(rate, vec(n0), 0.0, t, 600);
        expect(std::abs(pb::pivot_moment(out, g, 0.0).item<double>() / (1.0 - t / 2.0) - 1.0) < 2e-4, "product kernel: M0 = 1 - t/2");
        expect(std::abs(pb::pivot_moment(out, g, 2.0).item<double>() / (1.0 / (1.0 - t)) - 1.0) < 2e-3, "product kernel: M2 = 1/(1 - t)");
      }
    }
    // ---- aggregation on a coarse geometric pivot grid: the fixed pivot preserves number and mass, so N(t) = N0 / (1 + K N0 t / 2) exactly
    {
      const std::int64_t bins = 40;
      const auto g = pb::geometric_grid(1.0, 2.0, bins);
      const Series like = vec({0.0});
      const auto op = pb::build_aggregation_operator(g, like);
      std::vector<double> n0(static_cast<std::size_t>(bins), 0.0);
      n0[0] = 1.0;
      const Series kernel = vec(std::vector<double>(static_cast<std::size_t>(bins * bins), 1.0)).reshape({bins, bins});
      const dyn::RateFn rate = [&](const Series &y) { return pb::aggregation_rhs(y, op, kernel); };
      const double t = 20.0;
      const Series out = dyn::simulate(rate, vec(n0), 0.0, t, 1500);
      expect(std::abs(pb::pivot_moment(out, g, 0.0).item<double>() / (1.0 / (1.0 + t / 2.0)) - 1.0) < 1e-6, "fixed pivot: total number is exact");
      expect(std::abs(pb::pivot_moment(out, g, 1.0).item<double>() - 1.0) < 1e-9, "fixed pivot: mass is exact");
    }
    // ---- breakage: constant selection rate, uniform daughters beta = 2 / y: M_k' = (1 - k) / (1 + k) M_k
    {
      const std::int64_t bins = 36;
      const auto g = pb::geometric_grid(1.0, 1.5, bins);
      const Series like = vec({0.0});
      const auto op = pb::build_breakage_operator(g, [](double, double y) { return 2.0 / y; }, like);
      std::vector<double> sel(static_cast<std::size_t>(bins), 1.0);
      for (int i = 0; i < 6; ++i)
        sel[static_cast<std::size_t>(i)] = 0.0; // the smallest bins do not break: nothing is lost below the grid
      std::vector<double> n0(static_cast<std::size_t>(bins), 0.0);
      n0[30] = 1.0 / g.centers[30];
      const dyn::RateFn rate = [&](const Series &y) { return pb::breakage_rhs(y, op, vec(sel)); };
      const double t = 1.5;
      const Series out = dyn::simulate(rate, vec(n0), 0.0, t, 600);
      const double m1_0 = pb::pivot_moment(vec(n0), g, 1.0).item<double>();
      expect(std::abs(pb::pivot_moment(out, g, 1.0).item<double>() / m1_0 - 1.0) < 1e-12, "breakage conserves the mass exactly");
      const double m0_0 = pb::pivot_moment(vec(n0), g, 0.0).item<double>();
      expect(std::abs(pb::pivot_moment(out, g, 0.0).item<double>() / (m0_0 * std::exp(t)) - 1.0) < 2e-2, "breakage: M0 grows like exp(t) (fixed-pivot discretisation)");
      expect(pb::pivot_moment(out, g, 2.0).item<double>() < pb::pivot_moment(vec(n0), g, 2.0).item<double>(), "breakage narrows the distribution");
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "population balance tests passed\n";
  return 0;
}
