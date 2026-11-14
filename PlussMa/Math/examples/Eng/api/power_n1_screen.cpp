#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }


// five buses: a slack generator at bus 0, a second generator of 4 per unit at bus 3, and loads of 3, 3 and 4 per unit at buses 1, 2 and 4.
// Lines: 0: 0-1, 1: 0-3, 2: 0-4, 3: 1-2, 4: 2-3, 5: 3-4.
static power::Net five_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -3.0, -3.0, 0.0, -4.0});
  net.buses.q = vec({0.0, -0.6, -0.6, 0.0, -0.8});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {3};
  net.gens.p = vec({4.0});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 0, 0, 1, 2, 3};
  net.lines.to = {1, 3, 4, 2, 3, 4};
  net.lines.x = vec({0.028, 0.030, 0.006, 0.011, 0.030, 0.030});
  net.lines.r = vec({0.0028, 0.003, 0.0006, 0.0011, 0.003, 0.003});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const power::Grid grid = power::make(five_bus());
  const auto result = power::outages(grid);
  const std::vector<double> rating = {4.0, 3.0, 4.5, 4.0, 3.5, 4.1};
  const auto base = flat(result.base_flow);
  const auto after = flat(result.flow);

  out.writeln("base flows:  ", base);
  out.writeln("ratings:     ", rating);
  int violations = 0;
  for (int k = 0; k < 6; ++k)
    for (int m = 0; m < 6; ++m)
      if (m != k && std::abs(after[6 * k + m]) > rating[m]) {
        ++violations;
        out.writeln("losing line ", k, " overloads line ", m, ": flow ", after[6 * k + m], " against a rating of ", rating[m]);
      }
  out.writeln(violations, " overloads in all");
}
