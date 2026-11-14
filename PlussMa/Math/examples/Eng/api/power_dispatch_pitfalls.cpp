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

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto ratings = vec({10.0, 10.0, 10.0});

  // 1. the default accuracy is about five digits
  const auto rough = power::dispatch(grid, costs(), ratings);
  out.writeln("default solver: |g0 - 1| = ", std::abs(flat(rough.generation)[0] - 1.0), ", equal to 1 exactly: ", flat(rough.generation)[0] == 1.0);

  // 2. more demand than the generators can supply: the load is 1.5 and each unit is capped at 0.5
  power::GenCost small = costs();
  small.upper = vec({0.5, 0.5});
  const auto infeasible = power::dispatch(grid, small, ratings);
  out.writeln("demand 1.5, capacity 1.0: converged ", infeasible.converged, ", primal residual ", infeasible.primal_residual);

  // 3. a concave cost curve
  power::GenCost concave = costs();
  concave.quadratic = vec({-1.0, 8.0});
  try {
    (void)power::dispatch(grid, concave, ratings);
  } catch (const std::exception &e) {
    std::cout << "negative quadratic coefficient: " << e.what() << "\n";
  }

  // 4. ratings for the wrong number of lines
  try {
    (void)power::dispatch(grid, costs(), vec({10.0, 10.0}));
  } catch (const std::exception &e) {
    std::cout << "two ratings for three lines: " << e.what() << "\n";
  }
}
