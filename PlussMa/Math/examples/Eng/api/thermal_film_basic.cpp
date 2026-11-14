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
  // Convection resistance 1 / (h A): 25 W/(m^2 K) over 2 m^2.
  out.writeln("resistance", flat(th::film(vec({25.0}), vec({2.0}))), "K/W  closed form", 1.0 / 50.0);
  out.writeln("a sweep of h = 5, 25, 100 W/(m^2 K)", flat(th::film(vec({5.0, 25.0, 100.0}), vec({2.0}))), "K/W");
}
