// RANS closures coupled into the bounded incompressible stepper: a 2-D channel (no-slip wall at y = 0, symmetry at y = 1, open ends, body force 1)
// started from the converged one-dimensional channel solution must stay on it.
#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;
namespace rans = fluids::rans;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
auto metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
auto seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

// linear interpolation of a profile (y, f) at the points x
std::vector<double> interp(const std::vector<double> &y, const std::vector<double> &f, const std::vector<double> &x) {
  std::vector<double> out;
  for (const double v : x) {
    std::size_t i = 1;
    while (i + 1 < y.size() && y[i] < v) ++i;
    const double t = std::clamp((v - y[i - 1]) / (y[i] - y[i - 1]), 0.0, 1.0);
    out.push_back(f[i - 1] + t * (f[i] - f[i - 1]));
  }
  return out;
}
}  // namespace

int main(int argc, char **argv) {
  const double seconds_run = argc > 1 ? std::stod(argv[1]) : 0.2;
  try {
    const double re = 180.0, nu_v = 1.0 / re;
    const std::int64_t nx = 4, ny = 96;
    const double h = 1.0 / static_cast<double>(ny);
    int failures = 0;
    for (const auto model : {rans::Model::SpalartAllmaras, rans::Model::KOmegaSST}) {
      const std::string name = model == rans::Model::SpalartAllmaras ? "SA " : "SST";
      rans::ChannelOptions co;
      co.model = model;
      co.friction_reynolds = re;
      co.cells = ny;       // the same uniform mesh as the 2-D run, so both discretize the wall (omega at the first cell) identically
      co.stretching = 0.0;
      const auto ref = rans::solve_channel(co);
      std::vector<double> yc(static_cast<std::size_t>(ny));
      for (std::int64_t j = 0; j < ny; ++j) yc[static_cast<std::size_t>(j)] = (static_cast<double>(j) + 0.5) * h;
      const auto ry = ref.y.host<double>();
      const auto U = interp(ry, ref.velocity.host<double>(), yc), NUT = interp(ry, ref.eddy_viscosity.host<double>(), yc);
      const auto transported = model == rans::Model::SpalartAllmaras ? interp(ry, ref.nu_tilde.host<double>(), yc) : interp(ry, ref.k.host<double>(), yc);
      const auto omega = model == rans::Model::SpalartAllmaras ? std::vector<double>() : interp(ry, ref.omega.host<double>(), yc);

      const auto grid = spatial::make_box_grid({nx, ny}, {metres(h), metres(h)});
      const auto bounds = fluids::flow_bounds(grid, {fluids::outlet({0, spatial::Side::Low}), fluids::outlet({0, spatial::Side::High}), fluids::no_slip({1, spatial::Side::Low}),
                                                      fluids::free_slip({1, spatial::Side::High})});
      std::vector<double> u(static_cast<std::size_t>(2 * nx * ny), 0.0), t1(static_cast<std::size_t>(nx * ny)), t2(static_cast<std::size_t>(nx * ny));
      for (std::int64_t i = 0; i < nx; ++i)
        for (std::int64_t j = 0; j < ny; ++j) {
          u[static_cast<std::size_t>(i * ny + j)] = U[static_cast<std::size_t>(j)];
          t1[static_cast<std::size_t>(i * ny + j)] = transported[static_cast<std::size_t>(j)];
          if (!omega.empty()) t2[static_cast<std::size_t>(i * ny + j)] = omega[static_cast<std::size_t>(j)];
        }
      auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(pluss::series::make(u, {2, nx, ny}).to(F64)),
                                      common::SeriesQuantity<common::Pressure>::from_si(pluss::series::zeros({nx, ny}, F64)), grid);
      rans::Transport transport;
      if (model == rans::Model::SpalartAllmaras) {
        transport.nu_tilde = pluss::series::make(t1, {nx, ny}).to(F64);
      } else {
        transport.k = pluss::series::make(t1, {nx, ny}).to(F64);
        transport.omega = pluss::series::make(t2, {nx, ny}).to(F64);
      }
      const rans::Closure closure{.model = model};
      const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.0);
      const auto nu = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(nu_v);
      fluids::StepOptions opt;
      opt.rule = fluids::StepRule::Rk3;
      opt.guard.enforce = false;
      const fluids::BodyFn body = [&](common::ScalarQuantity<common::Time>, const fluids::IncompressibleState &) {
        const Series force = pluss::series::ones({nx, ny}, F64).unsqueeze(0).unite({pluss::series::zeros({nx, ny}, F64).unsqueeze(0)}, 0);
        return common::SeriesQuantity<common::Acceleration>::from_si(force);
      };
      const double dt = 4e-4;
      const std::int64_t steps = static_cast<std::int64_t>(seconds_run / dt);
      for (std::int64_t s = 0; s < steps; ++s) {
        const auto r = rans::advance(state, transport, grid, bounds, rho, nu, closure, seconds(dt * static_cast<double>(s)), seconds(dt), opt, body);
        state = r.flow.state;
        transport = r.transport;
      }
      const auto got = state.velocity().si_value().select(0, 0).select(0, 1).host<double>();  // u along y at x index 1
      double worst = 0.0;
      for (std::size_t j = 0; j < got.size(); ++j) worst = std::max(worst, std::abs(got[j] - U[j]) / U[j]);
      const auto nv = rans::eddy_viscosity(state, transport, grid, bounds, nu, closure).si_value().select(0, 1).host<double>();
      double worst_nut = 0.0;
      for (std::size_t j = static_cast<std::size_t>(ny / 8); j < static_cast<std::size_t>(ny - 4); ++j) worst_nut = std::max(worst_nut, std::abs(nv[j] - NUT[j]) / (NUT[j] + 1e-12));
      const double v_max = state.velocity().si_value().select(0, 1).abs().max().item<double>();
      std::cout << name << "  t = " << seconds_run << "  max relative velocity change " << worst << "  eddy-viscosity change (y > 1/8) " << worst_nut << "  max |v| " << v_max << "\n";
      if (!(worst < 0.03) || !(worst_nut < 0.15) || !(v_max < 1e-4)) {
        std::cout << "FAILED: " << name << " drifted from the channel solution\n";
        ++failures;
      }
    }
    std::cout << (failures ? "rans_coupled_test: failures\n" : "rans_coupled_test: all passed\n");
    return failures ? 1 : 0;
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
}
