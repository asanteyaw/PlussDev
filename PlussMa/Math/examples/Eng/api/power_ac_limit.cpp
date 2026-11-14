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

// the largest load the line can serve, from the radicand of the closed form
static double closed_form_limit() {
  const auto radicand = [](double P) {
    const double Q = 0.4 * P, r = 0.01, x = 0.1;
    const double b = 0.5 - (r * P + x * Q);
    return b * b - (r * r + x * x) * (P * P + Q * Q);
  };
  double lo = 0.1, hi = 10.0;
  for (int i = 0; i < 80; ++i) {
    const double mid = 0.5 * (lo + hi);
    (radicand(mid) >= 0.0 ? lo : hi) = mid;
  }
  return lo;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  power::Options options;
  options.max_iterations = 60;
  double lo = 0.5, hi = 6.0;   // converged at 0.5, not at 6
  for (int i = 0; i < 40; ++i) {
    const double mid = 0.5 * (lo + hi);
    (power::ac(power::make(two_bus(mid)), options).converged.item<bool>() ? lo : hi) = mid;
  }
  out.writeln("largest load for which the power flow converges (p.u.): ", lo);
  out.writeln("closed-form nose point (p.u.):                          ", closed_form_limit());
  for (double load : {1.0, 3.0, lo, 3.2}) {
    const auto r = power::ac(power::make(two_bus(load)), options);
    out.writeln("load ", load, ": converged ", r.converged.item<bool>(), ", load-bus voltage ", flat(r.magnitude)[1], ", residual ", r.residual.item<double>());
  }
}
