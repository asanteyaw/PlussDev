#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack at bus 0, a generator bus at bus 1 (1.02 p.u., 0.6 p.u. generation, 0.2 p.u. load) and a load bus at bus 2
static power::Net three_bus(power::BusKind second = power::BusKind::PV) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, second, power::BusKind::PQ};
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
  const power::Grid grid = power::make(three_bus());
  const auto r = power::ac(grid);
  out.writeln("voltage magnitudes: ", flat(r.magnitude), ", angles (rad): ", flat(r.angle));
  out.writeln("converged ", r.converged.item<bool>(), " in ", r.iterations, " iterations, residual ", r.residual.item<double>());

  // the defining equation, rebuilt from the voltages
  const auto vr = flat(r.voltage.real()), vi = flat(r.voltage.imag()), yr = flat(grid.ybus().real()), yi = flat(grid.ybus().imag());
  const auto pr = flat(r.injection.real()), pi = flat(r.injection.imag());
  double worst = 0.0;
  for (int i = 0; i < 3; ++i) {
    std::complex<double> current = 0.0;
    for (int j = 0; j < 3; ++j) current += std::complex<double>(yr[3 * i + j], yi[3 * i + j]) * std::complex<double>(vr[j], vi[j]);
    const std::complex<double> power = std::complex<double>(vr[i], vi[i]) * std::conj(current);
    worst = std::max(worst, std::abs(power - std::complex<double>(pr[i], pi[i])));
    out.writeln("bus ", i, ": reported ", pr[i], " + ", pi[i], "j    V conj(Y V) = ", power.real(), " + ", power.imag(), "j");
  }
  out.writeln("largest difference: ", worst);

  // losses: the sum of the two ends of every line
  const auto from = flat(r.from_power.real()), to = flat(r.to_power.real());
  double loss = 0.0, injected = 0.0;
  for (std::size_t line = 0; line < from.size(); ++line) loss += from[line] + to[line];
  for (double v : pr) injected += v;
  out.writeln("active losses from the line ends: ", loss, "   sum of the injections: ", injected);
}
