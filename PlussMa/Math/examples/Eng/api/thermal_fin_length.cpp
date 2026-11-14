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
  // A batch over fin length: the heat saturates and the efficiency falls as the fin gets longer.
  const double h = 25.0, k = 200.0, p = 0.016, a = 1.6e-5;
  const auto f = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({0.01, 0.03, 0.1, 0.3}), vec({100.0}), vec({20.0}));
  out.writeln("heat (W)  ", flat(f.heat));
  out.writeln("efficiency", flat(f.efficiency));
  const auto inf = th::fin(vec({h}), vec({k}), vec({p}), vec({a}), vec({1.0}), vec({100.0}), vec({20.0}), th::Tip::Infinite);
  out.writeln("limit for an infinitely long fin", flat(inf.heat), "W");
}
