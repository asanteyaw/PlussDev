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
  // Unequal flows: an economiser mixing outdoor air (-5 degrees, w = 0.002) into return air (22 degrees, w = 0.008). Fraction of outdoor air from 0 to 1.
  const pluss::Series outdoor = vec({0.0, 0.25, 0.5, 0.75, 1.0});
  const auto m = th::mix(vec({-5.0}), vec({0.002}), outdoor.add(1e-9), vec({22.0}), vec({0.008}), outdoor.neg().add(1.0).add(1e-9));
  out.writeln("outdoor fraction", flat(outdoor));
  out.writeln("mixed temperature", flat(m.temperature));
  out.writeln("mixed w          ", flat(m.w));
}
