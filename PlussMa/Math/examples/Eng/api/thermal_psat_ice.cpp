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
  // Below 0 degrees the vapour is in equilibrium with ice. The two published fits meet at 0 to within a fraction of a pascal.
  const pluss::Series t = vec({-40.0, -20.0, -10.0, -0.001, 0.0, 0.001});
  out.writeln("temperature", flat(t));
  out.writeln("psat       ", flat(th::psat(t)));
  const double below = one(th::psat(vec({-1e-9}))), above = one(th::psat(vec({1e-9})));
  out.writeln("just below 0:", below, " just above 0:", above, " jump", above - below, "Pa");
}
