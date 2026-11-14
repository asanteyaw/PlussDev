#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  power::Net spur = ring();
  spur.buses.kind.push_back(power::BusKind::PQ);   // a fourth bus on a spur from bus 2
  spur.buses.p = vec({0.0, 0.0, -0.5, -0.1});
  spur.buses.q = vec({0.0, 0.0, -0.1, -0.02});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(2);
  spur.lines.to.push_back(3);
  spur.lines.r = vec({0.01, 0.02, 0.03, 0.01});
  spur.lines.x = vec({0.1, 0.2, 0.3, 0.1});
  const power::Grid grid = power::make(spur);

  const auto result = power::outages(grid);
  out.writeln("valid outages (one entry per line): ", flat(result.valid));
  const auto after = flat(result.flow);
  for (int k = 0; k < 4; ++k) out.writeln("flows after losing line ", k, ": ", after[4 * k], " ", after[4 * k + 1], " ", after[4 * k + 2], " ", after[4 * k + 3]);
  out.writeln("with tolerance 0.9, valid outages: ", flat(power::lodf(grid, 0.9).valid));
}
