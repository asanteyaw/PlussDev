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
  // Equal mass flows of 30 degree air (w = 0.012) and 10 degree air (w = 0.006): w averages, enthalpy averages, temperature follows.
  const auto m = th::mix(vec({30.0}), vec({0.012}), vec({1.0}), vec({10.0}), vec({0.006}), vec({1.0}));
  out.writeln("mixed w", flat(m.w), " temperature", flat(m.temperature));
  out.writeln("enthalpy of the mixture", flat(th::enth(m.temperature, m.w)), " mean of the two", 0.5 * (one(th::enth(vec({30.0}), vec({0.012}))) + one(th::enth(vec({10.0}), vec({0.006})))));
}
