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
  const pluss::Series rows = pluss::make_series({1.0, 2.0, 4.0, 3.0, 5.0, 4.0, 6.0, 5.0, 10.0, 20.0, 40.0, 30.0, 50.0, 40.0, 60.0, 50.0}).reshape({2, 8});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const pluss::Series y = sig::zero_phase(rows, b, a, {.dim = 1, .edge = 3});
  out.writeln("output shape", y.size(0), "x", y.size(1));
  out.writeln("row 0 =", y.select(0, 0).items<double>());
  out.writeln("row 1 =", y.select(0, 1).items<double>(), " (ten times row 0)");
}
