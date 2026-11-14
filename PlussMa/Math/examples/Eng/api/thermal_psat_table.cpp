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
  // Saturation pressure of water vapour, in Pa, at familiar temperatures; the last is the boiling point at one atmosphere.
  const pluss::Series t = vec({0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 80.0, 100.0});
  out.writeln("psat", flat(th::psat(t)));
}
