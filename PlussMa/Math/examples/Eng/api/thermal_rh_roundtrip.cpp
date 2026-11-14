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
  // rh inverts ratio.
  const pluss::Series t = vec({5.0, 15.0, 25.0, 40.0});
  const pluss::Series w = th::ratio(t, vec({0.6}), vec({101325.0}));
  out.writeln("w", flat(w));
  out.writeln("rh back", flat(th::rh(t, w, vec({101325.0}))));
}
