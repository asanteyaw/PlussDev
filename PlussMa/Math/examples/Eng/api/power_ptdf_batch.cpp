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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Net outage = ring();
  outage.lines.active = {true, true, false};   // the direct line 0-2 is out
  const auto f = flat(power::ptdf(power::make(outage)));
  for (int line = 0; line < 3; ++line) out.writeln("line ", line, " (in service: ", line < 2 ? "yes" : "no ", "): ", f[3 * line], " ", f[3 * line + 1], " ", f[3 * line + 2]);

  // a batch of two load levels on the same network
  power::Net batch = ring();
  batch.buses.p = pluss::series::make(std::vector<double>{0, 0, -0.2, 0, 0, -0.5}).reshape({2, 3});
  batch.buses.q = pluss::series::make(std::vector<double>{0, 0, -0.04, 0, 0, -0.1}).reshape({2, 3});
  const auto factors = power::ptdf(power::make(batch));
  out.writeln("PTDF of a batch of two scenarios has shape ", factors.sizes());
  const auto both = flat(factors);
  double difference = 0.0;
  for (int i = 0; i < 9; ++i) difference = std::max(difference, std::abs(both[i] - both[9 + i]));
  out.writeln("largest difference between the matrices of the two scenarios: ", difference);
}
