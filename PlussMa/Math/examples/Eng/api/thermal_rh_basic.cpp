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
  // Relative humidity of air at 30 degrees with w = 0.010.
  const pluss::Series phi = th::rh(vec({30.0}), vec({0.010}), vec({101325.0}));
  out.writeln("relative humidity", flat(phi));
  out.writeln("warming the same air to 35 degrees lowers it:", flat(th::rh(vec({35.0}), vec({0.010}), vec({101325.0}))));
  out.writeln("cooling it to 10 degrees pushes it above 1:", flat(th::rh(vec({10.0}), vec({0.010}), vec({101325.0}))), "(supersaturated: it would condense)");
}
