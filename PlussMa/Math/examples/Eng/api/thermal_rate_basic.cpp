#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace th = pluss::subsp::eng::thermal;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static double one(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>()[0]; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // Hot water (1000 W/K) at 90 against cold water (2000 W/K) at 20 in a counter-flow exchanger with UA = 1500 W/K.
  const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), th::Flow::Counter);
  out.writeln("NTU", flat(r.ntu), " effectiveness", flat(r.effectiveness));
  out.writeln("duty", flat(r.duty), "W");
  out.writeln("hot out", flat(r.hot_out), " cold out", flat(r.cold_out));
  out.writeln("balance: hot gives", 1000.0 * (90.0 - one(r.hot_out)), " cold takes", 2000.0 * (one(r.cold_out) - 20.0));
}
