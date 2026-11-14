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
  // ntu inverts eff: size the exchanger for a required effectiveness of 0.8.
  const pluss::Series cr = vec({0.5});
  for (const auto flow : {th::Flow::Counter, th::Flow::Parallel, th::Flow::Shell, th::Flow::Cross}) {
    const char *name = flow == th::Flow::Counter ? "counter " : flow == th::Flow::Parallel ? "parallel" : flow == th::Flow::Shell ? "shell   " : "cross   ";
    const double target = flow == th::Flow::Parallel ? 0.6 : flow == th::Flow::Shell ? 0.7 : 0.8;
    const pluss::Series n = th::ntu(vec({target}), cr, flow);
    out.writeln(name, "effectiveness", target, " needs NTU", flat(n), " check eff =", flat(th::eff(n, cr, flow)));
  }
}
