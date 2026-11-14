#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // A pin fin, 4 mm square (P = 16 mm, A = 16 mm^2) of k = 200, 10 cm long, base at 100, air at 20 with h = 25. Adiabatic tip.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5, length = 0.1;
  const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({length}), vec({100.0}), vec({20.0}));
  const double m = std::sqrt(h * p / (k * a)), big_m = std::sqrt(h * p * k * a) * 80.0;
  out.writeln("m =", m, "1/m  m L =", m * length);
  out.writeln("heat from the base", flat(f.heat), "W  closed form M tanh(m L) =", big_m * std::tanh(m * length));
  out.writeln("tip temperature", flat(f.tip), " closed form", 20.0 + 80.0 / std::cosh(m * length));
  out.writeln("efficiency", flat(f.efficiency));
}
