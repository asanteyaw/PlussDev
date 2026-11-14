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
  // The same terminal temperatures differ by arrangement: parallel flow pairs hot-in with cold-in.
  out.writeln("counter ", flat(th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({40.0}), th::Flow::Counter)));
  out.writeln("parallel", flat(th::lmtd(vec({90.0}), vec({50.0}), vec({20.0}), vec({40.0}), th::Flow::Parallel)), " closed form", (70.0 - 10.0) / std::log(70.0 / 10.0));
}
