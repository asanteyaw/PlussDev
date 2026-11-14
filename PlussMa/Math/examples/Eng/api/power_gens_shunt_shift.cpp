#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto report = [&](const char *label, const power::Net &net) {
    const auto r = power::ac(power::make(net));
    out.writeln(label, ": slack output (p.u.) ", flat(r.injection.real())[0], ", flow on line 0-1 (p.u.) ", flat(r.from_power.real())[0], ", load-bus voltage ", flat(r.magnitude)[2]);
  };
  report("as described              ", three_bus());

  power::Net unit_out = three_bus();
  unit_out.gens.active = {false};              // the generator at bus 1 is out of service
  report("generator out of service  ", unit_out);

  power::Net conductance = three_bus();
  conductance.buses.g = vec({0.0, 0.0, 0.2});  // a resistive shunt at the load bus
  report("conductance 0.2 at bus 2  ", conductance);

  power::Net shifted = three_bus();
  shifted.lines.shift = vec({0.05, 0.0, 0.0}); // a phase shifter on line 0-1
  report("phase shift 0.05 on line 0", shifted);
}
