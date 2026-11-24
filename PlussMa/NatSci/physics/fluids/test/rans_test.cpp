// RANS closures on the turbulent channel: exact momentum balance, viscous sublayer, logarithmic layer, mixing-length eddy viscosity.
#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <iostream>
#include <string>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;
namespace rans = fluids::rans;
using pluss::Series;

namespace {
int failures = 0;
void check(bool ok, const std::string &what) { if (!ok) { std::cout << "FAILED: " << what << "\n"; ++failures; } }
// value of a profile at wall distance y+ (linear interpolation in log y+)
double at_yplus(const Series &y, const Series &f, double re_tau, double yplus) {
  const auto yv = y.host<double>(), fv = f.host<double>();
  for (std::size_t i = 1; i < yv.size(); ++i)
    if (yv[i] * re_tau >= yplus) { const double t = (yplus - yv[i - 1] * re_tau) / ((yv[i] - yv[i - 1]) * re_tau); return fv[i - 1] + t * (fv[i] - fv[i - 1]); }
  return fv.back();
}
}  // namespace

int main(int argc, char **argv) {
  const bool verbose = argc > 1;
  for (const auto model : {rans::Model::SpalartAllmaras, rans::Model::KOmegaSST}) {
    const std::string name = model == rans::Model::SpalartAllmaras ? "SA " : "SST";
    rans::ChannelOptions o;
    o.model = model;
    if (argc > 2) { o.growth_limit = std::stod(argv[2]); o.max_iterations = 20000; }
    const auto r = rans::solve_channel(o);
    const double re = o.friction_reynolds;
    std::cout << name << " iterations " << r.iterations << " residual " << r.residual << " converged " << r.converged << "\n";
    check(r.converged, name + " converged");
    // momentum balance: (nu + nu_t) dU/dy = 1 - y
    const Series balance = r.total_stress.sub(r.y.mul(-1.0).add(1.0)).abs();
    const double worst = balance.slice(0, 0, o.cells - 4).max().item<double>();
    check(worst < 2e-2, name + " momentum balance " + std::to_string(worst));
    const auto Up = [&](double yp) { return at_yplus(r.y, r.velocity, re, yp); };
    check(std::abs(Up(2.0) - 2.0) < 0.15, name + " viscous sublayer U+ = y+");
    for (const double yp : {50.0, 100.0, 200.0}) {
      const double log_law = std::log(yp) / 0.41 + 5.2;
      if (verbose) std::cout << "  y+ " << yp << "  U+ " << Up(yp) << "  log law " << log_law << "\n";
      check(std::abs(Up(yp) - log_law) < 0.8, name + " log law at y+ " + std::to_string(yp));
    }
    const double nut_ratio = at_yplus(r.y, r.eddy_viscosity, re, 100.0) * re / (0.41 * 100.0 * (1.0 - 100.0 / re));
    if (verbose) std::cout << "  nu_t / (kappa y+ (1-y) nu) at y+ = 100: " << nut_ratio << "  centreline U+ " << r.velocity.slice(0, o.cells - 1, o.cells).reshape({}).item<double>() << "\n";
    check(std::abs(nut_ratio - 1.0) < 0.3, name + " eddy viscosity follows the mixing length");
  }
  // a transport made for one closure must not be accepted by the other: the error names what is missing instead of failing inside the model
  {
    const auto h = common::ScalarQuantity<common::Length>::from_si(0.5);
    const auto grid = spatial::make_periodic_grid({8, 8}, {h, h});
    const Series zero = pluss::series::zeros({8, 8}, pluss::ScalarType::Float64);
    const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(pluss::series::zeros({2, 8, 8}, pluss::ScalarType::Float64)), common::SeriesQuantity<common::Pressure>::from_si(zero), grid);
    const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
    const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
    const auto time = common::ScalarQuantity<common::Time>::from_si(0.0), width = common::ScalarQuantity<common::Time>::from_si(0.01);
    rans::Closure sa, sst;
    sst.model = rans::Model::KOmegaSST;
    const auto sa_transport = rans::initial_transport(sa, viscosity, state), sst_transport = rans::initial_transport(sst, viscosity, state);
    const Series distance = zero.add(1.0);
    for (const auto &pair : {std::make_pair(sa, sst_transport), std::make_pair(sst, sa_transport)}) {
      bool refused = false;
      try {
        (void)rans::advance(state, pair.second, grid, density, viscosity, pair.first, time, width, {}, {}, distance);
      } catch (const pluss::Error &e) {
        refused = std::string(e.message()).find("needs a transport") != std::string::npos;
      }
      check(refused, "a transport made for the other closure is refused with a clear message");
    }
  }
  std::cout << (failures ? "rans_test: failures\n" : "rans_test: all passed\n");
  return failures ? 1 : 0;
}
