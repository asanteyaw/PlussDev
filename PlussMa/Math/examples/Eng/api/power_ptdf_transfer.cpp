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
  const auto f = flat(power::ptdf(grid));
  const int line = 0;
  const double shift = 0.1;   // generation moves from bus 1 to bus 2: the injection falls by 0.1 at bus 1 and rises by 0.1 at bus 2

  const double one_column = f[3 * line + 2] * shift;                           // reading the column of bus 2 alone
  const double difference = (f[3 * line + 2] - f[3 * line + 1]) * shift;        // the transfer from bus 1 to bus 2

  power::Net moved = ring();
  moved.buses.p = vec({0.0, -0.1, -0.4});
  const double before = flat(power::dc(grid).from_power)[line];
  const double after = flat(power::dc(power::make(moved)).from_power)[line];
  out.writeln("change of the flow on line 0 from re-solving:       ", after - before);
  out.writeln("column of bus 2 alone (wrong):                      ", one_column);
  out.writeln("difference of the columns of bus 2 and bus 1:       ", difference);
}
