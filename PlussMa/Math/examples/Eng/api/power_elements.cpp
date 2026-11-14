#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net two_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto voltage = [&](const power::Net &net) { return flat(power::ac(power::make(net)).magnitude)[1]; };

  out.writeln("plain line:                    load-bus voltage ", voltage(two_bus()));
  power::Net tapped = two_bus();
  tapped.lines.tap = vec({1.05});
  out.writeln("tap ratio 1.05:                load-bus voltage ", voltage(tapped));
  power::Net charged = two_bus();
  charged.lines.charge = vec({0.4});
  out.writeln("line charging 0.4:             load-bus voltage ", voltage(charged));
  power::Net shunted = two_bus();
  shunted.buses.b = vec({0.0, 0.3});
  out.writeln("shunt capacitor 0.3 at bus 1:  load-bus voltage ", voltage(shunted));
}
