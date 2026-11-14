#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: generators at buses 0 and 1, a load of 1.5 per unit at bus 2.
// Lines: 0 joins 0-1 (x = 0.1), 1 joins 1-2 (x = 0.2), 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -1.5});
  net.buses.q = vec({0.0, 0.0, -0.3});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {0, 1};
  net.gens.p = vec({0.5, 1.0});    // starting values: the dispatch chooses them
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

// generator 0: cost 5 g^2 + 10 g, generator 1: cost 8 g^2 + 12 g, each between 0 and 2 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({2.0, 2.0});
  c.linear = vec({10.0, 12.0});
  c.quadratic = vec({5.0, 8.0});
  return c;
}


// generator reactive limits of -1 and 1 per unit, bus voltages between 0.95 and 1.05, and no flow rating
static power::Limits limits() {
  power::Limits l;
  l.q_lower = vec({-1.0, -1.0});
  l.q_upper = vec({1.0, 1.0});
  l.vm_lower = vec({0.95, 0.95, 0.95});
  l.vm_upper = vec({1.05, 1.05, 1.05});
  return l;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto r = power::opf(grid, costs(), limits());
  out.writeln("converged ", r.converged, " in ", r.iterations, " iterations: ", r.message);
  const auto p = flat(r.generation_p), q = flat(r.generation_q);
  out.writeln("active output ", p, "   reactive output ", q, "   cost ", flat(r.objective));
  out.writeln("independent solver: active output [1.013894, 0.516653], reactive [0.214211, 0.391257], cost 23.614121");
  out.writeln("voltage magnitudes ", flat(r.magnitude), "   angles ", flat(r.angle));
  out.writeln("price of active power at the buses ", flat(r.p_price), "   price of reactive power ", flat(r.q_price));
  out.writeln("incremental cost of generator 0: ", 10.0 * p[0] + 10.0, "   of generator 1 at bus 1: ", 16.0 * p[1] + 12.0, "   against the prices at buses 0 and 1");
  out.writeln("losses (generation - demand): ", p[0] + p[1] - 1.5, " per unit");
  const auto dc = power::dispatch(grid, costs(), vec({10.0, 10.0, 10.0}));
  out.writeln("lossless DC dispatch: ", flat(dc.generation), ", cost ", flat(dc.objective));
}
