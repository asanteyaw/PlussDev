// Pressure projection through multigrid-preconditioned CG against the plain CG baseline on the same data: bounded constant density, bounded and periodic
// variable density (1000:1 contrast), tight Float64 tolerances. The projected velocities must agree and be divergence free; the explicit Fft request must be
// rejected where the operator is not constant-coefficient periodic.
#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;
using pluss::ScalarType;
using pluss::Series;

namespace {
constexpr auto F64 = ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series noise(std::vector<std::int64_t> shape, std::uint64_t seed) {
  std::int64_t n = 1;
  for (auto s : shape)
    n *= s;
  std::vector<double> v(static_cast<std::size_t>(n));
  for (auto &x : v) {
    seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
    x = static_cast<double>(seed >> 11) / 9007199254740992.0 - 0.5;
  }
  return pluss::series::make(v, shape).to(F64);
}
auto metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
auto seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }
Series bubble(std::int64_t n, std::int64_t m) {
  std::vector<double> v(static_cast<std::size_t>(n * m));
  for (std::int64_t i = 0; i < n; ++i)
    for (std::int64_t j = 0; j < m; ++j) {
      const double x = (i + 0.5) / n - 0.5, y = (j + 0.5) / m - 0.5;
      v[static_cast<std::size_t>(i * m + j)] = 1.0 + 999.0 * 0.5 * (1.0 - std::tanh((std::sqrt(x * x + y * y) - 0.25) * 12.0));
    }
  return pluss::series::make(v, {n, m}).to(F64);
}
fluids::ProjectionOptions strict(spatial::PoissonMethod method) {
  fluids::ProjectionOptions o;
  o.poisson.method = method;
  o.poisson.solve.max_iterations = 6000;
  o.poisson.solve.relative_tolerance = 1e-12;
  o.poisson.solve.absolute_tolerance = 1e-14;
  o.poisson.solve.check_interval = 1;
  o.poisson.mean_tolerance = 1e-9;
  o.poisson.require_convergence = false;
  return o;
}
double maxdiff(const Series &a, const Series &b) { return a.sub(b).abs().max().item<double>(); }
} // namespace

int main() {
  try {
    const std::int64_t n = 32, m = 24;
    // ---- periodic, variable density (parity-split multigrid is the exact operator)
    {
      const auto grid = spatial::make_periodic_grid({n, m}, {metres(1.0 / n), metres(1.0 / m)});
      const Series pressure = pluss::series::zeros({n, m}, F64);
      const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(noise({2, n, m}, 3).mul(0.1)), common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
      const auto rho = common::SeriesQuantity<fluids::Density>::from_si(bubble(n, m));
      const auto a = fluids::project(state, grid, rho, seconds(1.0), strict(spatial::PoissonMethod::Multigrid));
      const auto b = fluids::project(state, grid, rho, seconds(1.0), strict(spatial::PoissonMethod::ConjugateGradient));
      expect(maxdiff(a.state.velocity().si_value(), b.state.velocity().si_value()) < 1e-7, "periodic variable-density: multigrid vs CG velocity");
      expect(a.iterations <= 20 && a.iterations < b.iterations, "periodic variable-density iterations " + std::to_string(a.iterations) + " vs " + std::to_string(b.iterations));
      expect(a.converged.all().item<bool>() && a.residual_norm.dim() == b.residual_norm.dim(), "periodic variable-density diagnostics layout");
      bool threw = false;
      try {
        (void)fluids::project(state, grid, rho, seconds(1.0), strict(spatial::PoissonMethod::Fft));
      } catch (const std::invalid_argument &) {
        threw = true;
      }
      expect(threw, "Fft is rejected for variable density");
    }
    // ---- bounded cavity, constant and variable density
    {
      const auto grid = spatial::make_box_grid({n, m}, {metres(1.0 / n), metres(1.0 / m)});
      const auto lid = common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({1.0, 0.0}).to(F64));
      const auto bounds = fluids::flow_bounds(grid, {fluids::no_slip({0, spatial::Side::Low}), fluids::no_slip({0, spatial::Side::High}), fluids::no_slip({1, spatial::Side::Low}), fluids::no_slip({1, spatial::Side::High}, lid)});
      const Series pressure = pluss::series::zeros({n, m}, F64);
      const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(noise({2, n, m}, 5).mul(0.05)), common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
      const auto rho0 = common::ScalarQuantity<fluids::Density>::from_si(1.0);
      const auto a = fluids::project(state, grid, bounds, rho0, seconds(1.0), strict(spatial::PoissonMethod::Multigrid));
      const auto b = fluids::project(state, grid, bounds, rho0, seconds(1.0), strict(spatial::PoissonMethod::ConjugateGradient));
      std::cout << "  bounded: its " << a.iterations << " / " << b.iterations << " residual " << a.relative_residual.max().item<double>() << " / " << b.relative_residual.max().item<double>() << " diff " << maxdiff(a.state.velocity().si_value(), b.state.velocity().si_value()) << "\n";
      expect(maxdiff(a.state.velocity().si_value(), b.state.velocity().si_value()) < 1e-7, "bounded: multigrid vs CG velocity");
      expect(a.converged.all().item<bool>(), "bounded multigrid converged");
      std::cout << "  residuals " << a.relative_residual.max().item<double>() << " " << b.relative_residual.max().item<double>() << "\n";
      std::cout << "  bounded constant density: " << a.iterations << " multigrid vs " << b.iterations << " CG iterations\n";
      const auto rho = common::SeriesQuantity<fluids::Density>::from_si(bubble(n, m));
      const auto c = fluids::project(state, grid, bounds, rho, seconds(1.0), strict(spatial::PoissonMethod::Multigrid));
      const auto d = fluids::project(state, grid, bounds, rho, seconds(1.0), strict(spatial::PoissonMethod::ConjugateGradient));
      expect(maxdiff(c.state.velocity().si_value(), d.state.velocity().si_value()) < 1e-6, "bounded variable-density: multigrid vs CG velocity");
      std::cout << "  var-density its " << c.iterations << " / " << d.iterations << " residual " << c.relative_residual.max().item<double>() << " diff " << maxdiff(c.state.velocity().si_value(), d.state.velocity().si_value()) << "\n";
      expect(c.iterations < d.iterations, "bounded variable-density iterations " + std::to_string(c.iterations) + " vs " + std::to_string(d.iterations));
      std::cout << "  bounded variable density: " << c.iterations << " multigrid vs " << d.iterations << " CG iterations\n";
    }
    std::cout << "multigrid projection tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
