#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});      // the load at bus 1 is part of the bus; its generator is listed separately
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.gens.active = {true};
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});

  const auto r = power::ac(power::make(net));
  out.writeln("voltage magnitudes: ", flat(r.magnitude), "   angles (rad): ", flat(r.angle));
  out.writeln("net active injections: ", flat(r.injection.real()), "   (bus 1: -0.2 load + 0.6 generator = 0.4)");
  out.writeln("net reactive injections: ", flat(r.injection.imag()));
  out.writeln("converged ", r.converged.item<bool>(), " in ", r.iterations, " iterations");
}
