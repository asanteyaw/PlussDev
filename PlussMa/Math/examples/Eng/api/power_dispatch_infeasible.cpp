#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a square of four buses: generator A at bus 0 and generator B at bus 2, loads of 1 per unit at buses 1 and 3.
// Lines: 0 joins 0-1, 1 joins 1-2, 2 joins 2-3, 3 joins 3-0, each with x = 0.1.
static power::Net square() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -1.0, 0.0, -1.0});
  net.buses.q = vec({0.0, -0.2, 0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {0, 2};
  net.gens.p = vec({1.0, 1.0});
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 2, 3};
  net.lines.to = {1, 2, 3, 0};
  net.lines.r = vec({0.01, 0.01, 0.01, 0.01});
  net.lines.x = vec({0.1, 0.1, 0.1, 0.1});
  return net;
}

// generator A: 0.25 g^2 + 10 g; generator B: 0.25 g^2 + 10.5 g; each between 0 and 3 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({3.0, 3.0});
  c.linear = vec({10.0, 10.5});
  c.quadratic = vec({0.25, 0.25});
  return c;
}

static power::DispatchOptions tight(bool secure) {
  power::DispatchOptions options;
  options.secure = secure;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(square());
  const auto limits = vec({0.9, 0.9, 0.9, 0.9});
  const auto plain = power::dispatch(grid, costs(), limits, tight(false));
  const auto secure = power::dispatch(grid, costs(), limits, tight(true));
  out.writeln("plain dispatch at rating 0.9:  converged ", plain.converged, ", generation ", flat(plain.generation));
  out.writeln("secure dispatch at rating 0.9: converged ", secure.converged, ", primal residual ", secure.primal_residual);
}
