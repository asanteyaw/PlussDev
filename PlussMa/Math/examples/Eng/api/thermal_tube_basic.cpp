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
  // Insulation on a pipe: 25 mm to 50 mm radius, k = 0.04 W/(m K), 10 m long.
  const pluss::Series r = th::tube(vec({0.04}), vec({0.025}), vec({0.05}), vec({10.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", std::log(2.0) / (2.0 * M_PI * 0.04 * 10.0));
  out.writeln("heat loss for a 60 K difference", 60.0 / one(r), "W");
  const pluss::Series thicker = th::tube(vec({0.04}), vec({0.025}), vec({0.05, 0.075, 0.1}), vec({10.0}));
  out.writeln("outer radii 50, 75, 100 mm", flat(thicker), "K/W");
}
