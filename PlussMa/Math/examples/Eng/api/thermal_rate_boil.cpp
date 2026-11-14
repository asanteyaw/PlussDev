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
  // A boiler: the cold side changes phase at a fixed temperature, so only the hot stream cools and cr is zero.
  const auto r = th::rate(vec({800.0}), vec({1.0e9}), vec({300.0}), vec({100.0}), vec({400.0}), th::Flow::Boil);
  out.writeln("effectiveness", flat(r.effectiveness), " closed form", 1.0 - std::exp(-0.5));
  out.writeln("duty", flat(r.duty), "W  hot out", flat(r.hot_out), " cold out", flat(r.cold_out));
}
