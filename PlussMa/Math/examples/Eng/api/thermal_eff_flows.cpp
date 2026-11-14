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
  // The five arrangements at the same NTU = 2 and capacity ratio 0.5.
  const pluss::Series ntu = vec({2.0}), cr = vec({0.5});
  out.writeln("parallel", flat(th::eff(ntu, cr, th::Flow::Parallel)), " closed form", (1.0 - std::exp(-2.0 * 1.5)) / 1.5);
  out.writeln("counter ", flat(th::eff(ntu, cr, th::Flow::Counter)), " closed form", (1.0 - std::exp(-2.0 * 0.5)) / (1.0 - 0.5 * std::exp(-2.0 * 0.5)));
  out.writeln("cross   ", flat(th::eff(ntu, cr, th::Flow::Cross)));
  out.writeln("shell   ", flat(th::eff(ntu, cr, th::Flow::Shell)));
  out.writeln("boil    ", flat(th::eff(ntu, cr, th::Flow::Boil)), " closed form", 1.0 - std::exp(-2.0), " (cr plays no part)");
}
