#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  const pluss::Series b = pluss::make_series({0.5, 0.3, 0.2});
  const pluss::Series a = pluss::make_series({1.0, -0.4, 0.1});
  const sig::FilterRun run = sig::filter(x, b, a);
  out.writeln("values length", run.values.numel(), " state length", run.state.numel(), " for a filter of order 2");
  out.writeln("values =", run.values.items<double>());
  out.writeln("state  =", run.state.items<double>());
}
