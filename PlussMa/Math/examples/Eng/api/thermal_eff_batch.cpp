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
  // A grid of NTU (rows) against capacity ratio (columns) for counter flow in one call.
  const pluss::Series ntu = vec({0.5, 1.0, 2.0, 4.0}).reshape({4, 1});
  const pluss::Series cr = vec({0.0, 0.5, 1.0}).reshape({1, 3});
  const pluss::Series e = th::eff(ntu, cr, th::Flow::Counter);
  out.writeln("shape", e.sizes(), " (ntu, cr)");
  for (int row = 0; row < 4; ++row) out.writeln("ntu", flat(ntu)[row], flat(e.select(0, row)));
}
