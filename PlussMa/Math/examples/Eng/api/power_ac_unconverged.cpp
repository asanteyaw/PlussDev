#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack and a load bus joined by a line of 0.01 + 0.1j per unit; the load has a power factor of about 0.93
static power::Net two_bus(double active) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -active});
  net.buses.q = vec({0.0, -0.4 * active});
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
  power::Options options;
  options.max_iterations = 60;
  const auto r = power::ac(power::make(two_bus(3.2)), options);
  out.writeln("converged: ", r.converged.item<bool>(), ", residual (p.u.): ", r.residual.item<double>());
  out.writeln("load-bus voltage ", flat(r.magnitude)[1], ", load-bus angle ", flat(r.angle)[1]);
  out.writeln("reported load-bus injection: ", flat(r.injection.real())[1], " + ", flat(r.injection.imag())[1], "j   requested: -3.2 - 1.28j");
}
