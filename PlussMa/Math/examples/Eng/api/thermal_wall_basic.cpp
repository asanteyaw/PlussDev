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
  // A concrete wall, 0.2 m thick, k = 1.4 W/(m K), 10 m^2: R = L / (k A).
  const pluss::Series r = th::wall(vec({1.4}), vec({0.2}), vec({10.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", 0.2 / (1.4 * 10.0));
  out.writeln("heat flow for 20 K across the wall", 20.0 / one(r), "W");
  const pluss::Series sweep = th::wall(vec({1.4}), vec({0.1, 0.2, 0.4}), vec({10.0}));
  out.writeln("three thicknesses (broadcast)", flat(sweep), "K/W");
}
