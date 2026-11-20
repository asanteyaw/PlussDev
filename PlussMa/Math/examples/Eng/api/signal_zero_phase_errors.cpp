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
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  refused(out, "an input shorter than the edge extension", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0}), b, a, {.dim = 0, .edge = 5}); });
  refused(out, "a negative edge other than -1", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), b, a, {.dim = 0, .edge = -2}); });
  refused(out, "an axis out of range", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), b, a, {.dim = 4}); });
}
