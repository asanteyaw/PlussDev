#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(two_bus(0.5));
  struct Run { const char *name; double tolerance; std::int64_t iterations; };
  for (const Run &run : {Run{"defaults (1e-10, 20)  ", 1e-10, 20}, Run{"tolerance 1e-3        ", 1e-3, 20}, Run{"tolerance 1e-14       ", 1e-14, 20}, Run{"max_iterations 2      ", 1e-10, 2}}) {
    power::Options options;
    options.tolerance = run.tolerance;
    options.max_iterations = run.iterations;
    const auto r = power::ac(grid, options);
    out.writeln(run.name, ": iterations ", r.iterations, ", residual ", r.residual.item<double>(), ", converged ", r.converged.item<bool>());
  }
}
