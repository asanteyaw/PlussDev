// A-posteriori LES validation on the 3-D Taylor-Green vortex at Re = 1600 (quick) or 800 (full): the kinetic-energy history of a coarse run with and without a
// subgrid model against a fine run of the same flow. `les_validation_test full` runs 32^3 against 64^3 at Re = 400 to t = 4; the default is a quick 16^3 / 32^3 at Re = 1600 to t = 3.
#include "pluss/ma/natsci/physics.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;
namespace les = fluids::les;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
auto metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
auto seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

struct Run { std::vector<double> time, kinetic; double max_divergence = 0.0; };

Run simulate(std::int64_t n, const les::Closure &closure, double nu_value, double t_end, double dt, int samples) {
  const double h = 2.0 * std::numbers::pi / static_cast<double>(n);
  const auto grid = spatial::make_periodic_grid({n, n, n}, {metres(h), metres(h), metres(h)});
  std::vector<double> u(static_cast<std::size_t>(3 * n * n * n), 0.0);
  const auto at = [&](std::int64_t c, std::int64_t i, std::int64_t j, std::int64_t k) -> double & { return u[static_cast<std::size_t>(((c * n + i) * n + j) * n + k)]; };
  for (std::int64_t i = 0; i < n; ++i)
    for (std::int64_t j = 0; j < n; ++j)
      for (std::int64_t k = 0; k < n; ++k) {
        const double x = (i + 0.5) * h, y = (j + 0.5) * h, z = (k + 0.5) * h;
        at(0, i, j, k) = std::sin(x) * std::cos(y) * std::cos(z);
        at(1, i, j, k) = -std::cos(x) * std::sin(y) * std::cos(z);
      }
  auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(pluss::series::make(u, {3, n, n, n}).to(F64)),
                                  common::SeriesQuantity<common::Pressure>::from_si(pluss::series::zeros({n, n, n}, F64)), grid);
  const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto nu = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(nu_value);
  fluids::StepOptions opt;
  opt.rule = fluids::StepRule::Rk3;
  opt.guard.enforce = false;
  opt.projection.poisson.mean_tolerance = 1e-5;
  state = fluids::project(state, grid, rho, seconds(1.0), opt.projection).state;
  const auto kinetic = [&](const fluids::IncompressibleState &s) { return s.velocity().si_value().square().sum().mul(0.5).item<double>() / static_cast<double>(n * n * n); };
  Run run;
  const std::int64_t steps = static_cast<std::int64_t>(std::llround(t_end / dt)), every = steps / samples;
  run.time.push_back(0.0);
  run.kinetic.push_back(kinetic(state));
  for (std::int64_t s = 1; s <= steps; ++s) {
    const auto r = les::advance(state, grid, rho, nu, closure, seconds(dt * (s - 1)), seconds(dt), opt);
    state = r.state;
    run.max_divergence = std::max(run.max_divergence, r.divergence.field.si_value().abs().max().item<double>());
    if (s % every == 0) { run.time.push_back(dt * s); run.kinetic.push_back(kinetic(state)); }
  }
  return run;
}
} // namespace

int main(int argc, char **argv) {
  const bool full = argc > 1 && std::string(argv[1]) == "full";
  const std::int64_t coarse = full ? 32 : 16, fine = full ? 64 : 32;
  const double t_end = full ? 4.0 : 3.0, nu = full ? 1.0 / 400.0 : 1.0 / 1600.0, dt = full ? 0.01 : 0.02;
  const int samples = 4;
  const auto clock = [] { return std::chrono::steady_clock::now(); };
  const auto since = [&](auto t0) { return std::chrono::duration<double>(clock() - t0).count(); };
  try {
    auto t0 = clock();
    const Run reference = simulate(fine, les::Closure{.model = les::Model::Smagorinsky, .constant = 0.0}, nu, t_end, dt, samples);
    std::cout << "reference " << fine << "^3 done in " << since(t0) << " s\n";
    struct Case { const char *name; les::Closure closure; };
    const Case cases[] = {{"no model ", {.model = les::Model::Smagorinsky, .constant = 0.0}}, {"Smagorinsky", {.model = les::Model::Smagorinsky}},
                          {"WALE       ", {.model = les::Model::Wale}}, {"dynamic    ", {.model = les::Model::DynamicSmagorinsky}}};
    std::cout << "t            ";
    for (double t : reference.time) std::cout << t << "      ";
    std::cout << "\nreference    ";
    for (double k : reference.kinetic) std::cout << k << "  ";
    std::cout << "\n";
    double baseline = 0.0;
    int status = 0;
    for (const Case &c : cases) {
      t0 = clock();
      const Run r = simulate(coarse, c.closure, nu, t_end, dt, samples);
      std::cout << c.name << "  ";
      double worst = 0.0;
      for (std::size_t i = 0; i < r.kinetic.size(); ++i) {
        std::cout << r.kinetic[i] << "  ";
        worst = std::max(worst, std::abs(r.kinetic[i] - reference.kinetic[i]) / reference.kinetic[i]);
      }
      std::cout << "  worst relative error " << worst << "  max div " << r.max_divergence << "  (" << since(t0) << " s)\n";
      if (&c == &cases[0]) baseline = worst;
      else if (c.closure.model != les::Model::Wale && !(worst < baseline)) { std::cout << "FAILED: " << c.name << "does not improve on the unmodelled run\n"; status = 1; }
      if (r.max_divergence > 1e-8) { std::cout << "FAILED: closure step not divergence free\n"; status = 1; }
    }
    return status;
  } catch (const std::exception &e) { std::cerr << "FAILED: " << e.what() << "\n"; return 1; }
}
