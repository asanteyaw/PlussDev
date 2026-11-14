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
  // Cross flow has no closed-form inverse, so it is found by root bracketing; the batch is solved together.
  const pluss::Series target = vec({0.2, 0.5, 0.7, 0.9});
  const pluss::Series n = th::ntu(target, vec({0.75}), th::Flow::Cross);
  out.writeln("NTU needed", flat(n));
  out.writeln("effectiveness back", flat(th::eff(n, vec({0.75}), th::Flow::Cross)));
}
