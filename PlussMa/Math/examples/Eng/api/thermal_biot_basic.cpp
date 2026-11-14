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
  // Bi = h L / k compares the surface resistance with the internal one. Copper, steel and a brick in the same air stream.
  const pluss::Series k = vec({400.0, 15.0, 0.7});
  const pluss::Series bi = th::biot(vec({50.0}), k, vec({0.02}));
  out.writeln("Biot numbers", flat(bi), " for copper, steel, brick (h = 50, L = 0.02 m)");
  out.writeln("lumped treatment is sound below about 0.1:", flat(bi.lt(0.1)));
}
