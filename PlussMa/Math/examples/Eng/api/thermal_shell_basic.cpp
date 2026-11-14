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
  // A spherical shell of k = 1 from radius 1 m to 2 m: R = (1 / r1 - 1 / r2) / (4 pi k).
  const pluss::Series r = th::shell(vec({1.0}), vec({1.0}), vec({2.0}));
  out.writeln("resistance", flat(r), "K/W  closed form", 0.5 / (4.0 * M_PI));
  out.writeln("an infinite outer radius leaves 1 / (4 pi k r1) =", 1.0 / (4.0 * M_PI), "K/W");
  out.writeln("outer radii 1.5, 2, 4, 100 m", flat(th::shell(vec({1.0}), vec({1.0}), vec({1.5, 2.0, 4.0, 100.0}))), "K/W");
}
