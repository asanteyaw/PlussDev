// McKendrick-von Foerster / Gurtin-MacCamy by characteristics: exact transport, second-order convergence to the continuous Euler-Lotka rate, and the closed-form
// density-dependent equilibrium.
#include "pluss/subsp/natsci/biology/population/age_structured.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace as = pluss::subsp::natsci::biology::population::age_structured;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
// n0(a) = exp(-(a - 1)^2) shifted by the exact characteristic
double gaussian(double a) { return std::exp(-(a - 1.0) * (a - 1.0)); }
} // namespace

int main() {
  try {
    // ---- exact transport with constant mortality and no births: n(a, t) = n0(a - t) e^{-mu t}
    {
      const as::Grid grid{.step = 0.05, .nodes = 400};
      const as::Model model{.mortality = [](const Series &a, const Series &) { return a.mul(0.0).add(0.3); }, .fecundity = [](const Series &a, const Series &) { return a.mul(0.0); }};
      std::vector<double> init;
      for (std::int64_t j = 0; j < grid.nodes; ++j) init.push_back(gaussian(static_cast<double>(j) * grid.step));
      const std::int64_t k = 60;
      const Series n = as::simulate(vec(init), model, grid, k);
      const auto v = n.contiguous().host<double>();
      double worst = 0.0;
      for (std::int64_t j = 0; j < grid.nodes; ++j) {
        const double want = j >= k ? gaussian(static_cast<double>(j - k) * grid.step) * std::exp(-0.3 * static_cast<double>(k) * grid.step) : 0.0;
        worst = std::max(worst, std::abs(v[static_cast<std::size_t>(j)] - want));
      }
      expect(worst < 1e-14, "exact characteristic transport and mortality, worst " + std::to_string(worst));
    }
    // ---- intrinsic growth rate: exponential fecundity beta0 e^{-c a}, constant mortality mu => r = beta0 - c - mu; second-order in the step
    {
      constexpr double beta0 = 3.0, c = 0.5, mu = 0.4, r_exact = beta0 - c - mu;
      std::vector<double> errors;
      for (const double h : {0.08, 0.04, 0.02}) {
        const as::Grid grid{.step = h, .nodes = static_cast<std::int64_t>(14.0 / h)};
        const as::Model model{.mortality = [=](const Series &a, const Series &) { return a.mul(0.0).add(mu); },
                              .fecundity = [=](const Series &a, const Series &) { return a.mul(-c).exp().mul(beta0); }};
        Series n = vec(std::vector<double>(static_cast<std::size_t>(grid.nodes), 0.0));
        n = n.add(grid.ages().mul(-1.0).exp());
        const auto steps = static_cast<std::int64_t>(6.0 / h);
        n = as::simulate(n, model, grid, steps);
        const Series next = as::step(n, model, grid);
        const double rate = std::log(as::total(next, grid).item<double>() / as::total(n, grid).item<double>()) / h;
        errors.push_back(std::abs(rate - r_exact));
      }
      expect(errors[0] > errors[1] && errors[1] > errors[2], "the growth-rate error decreases with the step");
      expect(errors[0] / errors[1] > 3.5 && errors[1] / errors[2] > 3.5, "second-order convergence: ratios " + std::to_string(errors[0] / errors[1]) + ", " + std::to_string(errors[1] / errors[2]));
      expect(errors[2] < 1e-3, "finest step is within 1e-3 of r = beta0 - c - mu: " + std::to_string(errors[2]));
      // the continuous Euler-Lotka solver on the same nodes agrees (both converge to r)
      const as::Grid grid{.step = 0.02, .nodes = 700};
      const Series ages = grid.ages();
      const auto lt = as::euler_lotka(ages.mul(0.0).add(mu), ages.mul(-c).exp().mul(beta0), grid);
      expect(std::abs(lt.r.item<double>() - r_exact) < 2e-3, "trapezoid Euler-Lotka rate: " + std::to_string(lt.r.item<double>()));
      expect(std::abs(lt.r0.item<double>() - beta0 / (c + mu)) < 2e-3, "R0 = beta0 / (c + mu)");
    }
    // ---- Gurtin-MacCamy: mu = mu0 + mu1 N, beta = beta0 e^{-c a}  =>  N* = (beta0 - c - mu0) / mu1, B* = (beta0 - c) N*
    {
      constexpr double beta0 = 3.0, c = 0.5, mu0 = 0.4, mu1 = 0.05, nstar = (beta0 - c - mu0) / mu1;
      const as::Grid grid{.step = 0.02, .nodes = 900};
      const as::Model model{.mortality = [=](const Series &a, const Series &n) { return a.mul(0.0).add(n.unsqueeze(-1).mul(mu1).add(mu0)); },
                            .fecundity = [=](const Series &a, const Series &) { return a.mul(-c).exp().mul(beta0); },
                            .density_dependent = true};
      Series n = grid.ages().mul(-0.7).exp().mul(3.0);
      for (int block = 0; block < 4; ++block) n = as::simulate(n, model, grid, 1500);
      const double got = as::total(n, grid).item<double>();
      expect(std::abs(got - nstar) < 2e-3 * nstar, "Gurtin-MacCamy equilibrium population " + std::to_string(got) + " vs " + std::to_string(nstar));
      expect(std::abs(n.select(-1, 0).item<double>() / ((beta0 - c) * nstar) - 1.0) < 3e-3, "equilibrium birth rate B* = (beta0 - c) N*");
      // the equilibrium age profile is exponential with rate beta0 - c
      const double a = 50 * grid.step;
      expect(std::abs(n.select(-1, 50).item<double>() / n.select(-1, 0).item<double>() - std::exp(-(beta0 - c) * a)) < 1e-3, "equilibrium age distribution e^{-(beta0 - c) a}");
      // two lanes (different initial populations) converge to the same equilibrium in one batched call
      const Series two = grid.ages().mul(-0.7).exp().unsqueeze(0).mul(vec({1.0, 12.0}).unsqueeze(-1));
      Series m = two;
      for (int block = 0; block < 4; ++block) m = as::simulate(m, model, grid, 1500);
      const Series nt = as::total(m, grid);
      expect(std::abs(nt.select(0, 0).item<double>() - nt.select(0, 1).item<double>()) < 5e-3 && std::abs(nt.select(0, 1).item<double>() - nstar) < 2e-3 * nstar, "batched lanes reach the same equilibrium");
    }
    std::cout << "age_structured tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
