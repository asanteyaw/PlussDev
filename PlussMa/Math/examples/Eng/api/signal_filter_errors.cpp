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
  refused(out, "a zero leading denominator coefficient", [] { sig::filter(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}), pluss::make_series({0.0, 1.0})); });
  refused(out, "an integer input", [] { sig::filter(pluss::make_series({1, 2}), pluss::make_series({1.0}), pluss::make_series({1.0})); });
  refused(out, "a state shorter than the filter order", [] {
    const auto first = sig::filter(pluss::make_series({1.0, 2.0, 3.0}), pluss::make_series({1.0, 1.0}), pluss::make_series({1.0}));
    sig::filter(pluss::make_series({1.0, 2.0, 3.0}), pluss::make_series({1.0, 1.0, 1.0}), pluss::make_series({1.0}), first.state);
  });
}
