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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  // The moist-air functions take degrees Celsius; the property provider takes kelvin. Passing kelvin to a psychrometric call is refused.
  const double kelvin = 293.15;
  report([&] { (void)th::psat(vec({kelvin})); });
  out.writeln("psat at 20 degrees Celsius:", flat(th::psat(vec({kelvin - 273.15}))), "Pa");
  report([&] { (void)th::props(th::Fluid::Water, vec({20.0})); });
  out.writeln("water density at 293.15 K:", flat(th::props(th::Fluid::Water, vec({kelvin})).density), "kg/m^3");
}
