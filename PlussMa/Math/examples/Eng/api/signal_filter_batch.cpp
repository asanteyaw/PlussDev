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
  const pluss::Series rows = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}).reshape({2, 4});
  const sig::FilterRun run = sig::filter(rows, pluss::make_series({1.0}), pluss::make_series({1.0, -0.5}), 1);
  out.writeln("values shape", run.values.size(0), "x", run.values.size(1), " state shape", run.state.size(0), "x", run.state.size(1));
  out.writeln("row 0 =", run.values.select(0, 0).items<double>());
  out.writeln("row 1 =", run.values.select(0, 1).items<double>(), " (ten times row 0)");
}
