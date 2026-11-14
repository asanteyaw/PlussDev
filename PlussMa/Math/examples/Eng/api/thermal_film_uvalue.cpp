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
  // The overall coefficient of a wall: films on both faces plus the wall itself, U = 1 / (A R_total).
  const double area = 12.0;
  const pluss::Series inside = th::film(vec({8.0}), vec({area}));
  const pluss::Series outside = th::film(vec({25.0}), vec({area}));
  const pluss::Series wall = th::wall(vec({0.5}), vec({0.3}), vec({area}));
  const double total = one(inside) + one(wall) + one(outside);
  out.writeln("resistances: inside", flat(inside), " wall", flat(wall), " outside", flat(outside), "K/W");
  out.writeln("U =", 1.0 / (area * total), "W/(m^2 K); heat flow for 25 K", 25.0 / total, "W");
  out.writeln("without the films the wall alone would pass", 25.0 / one(wall), "W");
}
