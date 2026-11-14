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
  // Balanced counter flow (cr = 1) has the closed form NTU / (1 + NTU); the limit is smooth, not a special case.
  out.writeln("cr = 1      ", flat(th::eff(vec({0.5, 1.0, 3.0, 10.0}), vec({1.0}), th::Flow::Counter)), " closed form", 0.5 / 1.5, 1.0 / 2.0, 3.0 / 4.0, 10.0 / 11.0);
  out.writeln("cr = 0.9999 ", flat(th::eff(vec({0.5, 1.0, 3.0, 10.0}), vec({0.9999}), th::Flow::Counter)));
  out.writeln("cr = 0      ", flat(th::eff(vec({0.5, 1.0, 3.0, 10.0}), vec({0.0}), th::Flow::Counter)), " all arrangements agree here: 1 - exp(-NTU)");
}
