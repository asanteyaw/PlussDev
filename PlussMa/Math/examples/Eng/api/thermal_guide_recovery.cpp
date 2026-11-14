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
  // A heat-recovery ventilator: exhaust air at 22 degrees and 40 % warms outdoor air at -5 degrees and 80 %; 0.1 kg/s of dry air each way.
  const double p = 101325.0, flow = 0.1;
  const pluss::Series w_out = th::ratio(vec({-5.0}), vec({0.8}), vec({p})), w_in = th::ratio(vec({22.0}), vec({0.4}), vec({p}));
  const double c_out = flow * (1006.0 + 1860.0 * one(w_out)), c_in = flow * (1006.0 + 1860.0 * one(w_in));
  const auto r = th::rate(vec({c_in}), vec({c_out}), vec({22.0}), vec({-5.0}), vec({120.0}), th::Flow::Counter);
  out.writeln("capacity rates: exhaust", c_in, "W/K  outdoor air", c_out, "W/K");
  out.writeln("effectiveness", flat(r.effectiveness), " supply air", flat(r.cold_out), " exhaust leaves at", flat(r.hot_out));
  const pluss::Series dew = th::dew(w_in, vec({p}));
  out.writeln("dew point of the exhaust", flat(dew), "-> the exchanger wall is colder than that, so the exhaust condenses:", one(r.hot_out) < one(dew));
  out.writeln("heat recovered (sensible)", flat(r.duty), "W");
}
