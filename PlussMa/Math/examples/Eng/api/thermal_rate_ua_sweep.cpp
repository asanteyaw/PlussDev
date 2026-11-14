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
  // A sweep of the conductance UA: the duty saturates at Cmin (T_hot - T_cold).
  const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({100.0, 500.0, 1500.0, 5000.0, 50000.0}), th::Flow::Counter);
  out.writeln("duty", flat(r.duty), "W");
  out.writeln("ceiling Cmin dT =", 1000.0 * 70.0, "W");
  out.writeln("hot out", flat(r.hot_out));
}
