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
  // The same duty question in each arrangement: counter flow does the most, parallel the least.
  for (const auto flow : {th::Flow::Parallel, th::Flow::Cross, th::Flow::Shell, th::Flow::Counter}) {
    const auto r = th::rate(vec({1000.0}), vec({2000.0}), vec({90.0}), vec({20.0}), vec({1500.0}), flow);
    const char *name = flow == th::Flow::Parallel ? "parallel" : flow == th::Flow::Cross ? "cross   " : flow == th::Flow::Shell ? "shell   " : "counter ";
    out.writeln(name, "duty", flat(r.duty), "W  hot out", flat(r.hot_out), " cold out", flat(r.cold_out));
  }
}
