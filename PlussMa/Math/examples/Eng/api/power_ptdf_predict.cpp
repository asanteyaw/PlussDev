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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto base = flat(power::dc(grid).from_power);
  const auto f = flat(power::ptdf(grid));

  const double change = -0.3;   // the injection at bus 2 falls by 0.3: a larger load
  std::vector<double> predicted(3);
  for (int line = 0; line < 3; ++line) predicted[line] = base[line] + f[3 * line + 2] * change;

  power::Net heavier = ring();
  heavier.buses.p = vec({0.0, 0.0, -0.8});
  const auto resolved = flat(power::dc(power::make(heavier)).from_power);
  out.writeln("base flows:               ", base);
  out.writeln("predicted with the PTDF:  ", predicted);
  out.writeln("re-solved flows:          ", resolved);
  double worst = 0.0;
  for (int line = 0; line < 3; ++line) worst = std::max(worst, std::abs(predicted[line] - resolved[line]));
  out.writeln("largest difference: ", worst);
}
