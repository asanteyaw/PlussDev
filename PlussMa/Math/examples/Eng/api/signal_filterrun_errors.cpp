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
  const pluss::Series b = pluss::make_series({0.5, 0.5});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  const pluss::Series b3 = pluss::make_series({0.5, 0.3, 0.2});
  const sig::FilterRun first = sig::filter(pluss::make_series({1.0, 2.0, 3.0}), b, a);
  refused(out, "a state from a filter of a different order", [&] { sig::filter(pluss::make_series({1.0, 2.0, 3.0}), b3, a, first.state); });
  refused(out, "a state of the right order", [&] { sig::filter(pluss::make_series({4.0, 5.0, 6.0}), b, a, first.state); });
  // One state is shared by every lane: it broadcasts over a batch of rows.
  const sig::FilterRun rows = sig::filter(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3}), b, a, first.state, 1);
  out.writeln("one state shared by two rows: state shape", rows.state.size(0), "x", rows.state.size(1));
}
