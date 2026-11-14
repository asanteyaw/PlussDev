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
  // Hot 90 -> 50, cold 20 -> 60 (counter flow): the end differences are 30 and 30, so the log mean is 30.
  out.writeln("equal ends", flat(th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({60.0}), th::Flow::Counter)));
  // Hot 90 -> 40, cold 20 -> 50: the end differences are 40 and 20.
  out.writeln("unequal ends", flat(th::lmtd(vec({90.0}), vec({40.0}), vec({20.0}), vec({50.0}), th::Flow::Counter)), " closed form", 20.0 / std::log(2.0));
}
