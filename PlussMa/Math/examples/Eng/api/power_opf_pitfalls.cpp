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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(ring());

  // 1. voltage limits the network cannot meet: no point satisfies them
  power::Limits tight_voltage = limits();
  tight_voltage.vm_lower = vec({0.98, 0.98, 0.98});
  const auto a = power::opf(grid, costs(), tight_voltage);
  out.writeln("voltages >= 0.98: converged ", a.converged, ", violation ", a.constraint_violation, ", cost reported ", flat(a.objective), " (not a result)");
  std::cout << "  message: " << a.message << "\n";

  // 2. generator capacity of 1.0 for a demand of 1.5
  power::GenCost small = costs();
  small.upper = vec({0.5, 0.5});
  const auto b = power::opf(grid, small, limits());
  out.writeln("capacity 1.0 for demand 1.5: converged ", b.converged, ", violation ", b.constraint_violation);

  // 3. a lower voltage bound above the upper bound
  power::Limits crossed = limits();
  crossed.vm_lower = vec({1.1, 1.1, 1.1});
  try {
    (void)power::opf(grid, costs(), crossed);
  } catch (const std::exception &e) {
    std::cout << "lower bound above the upper bound: " << e.what() << "\n";
  }
}
