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
  refused(out, "Odd with an edge as long as the input", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0}), b, a, {.dim = 0, .edge = 4, .pad = sig::PadMode::Odd}); });
  refused(out, "None ignores the edge length", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0}), b, a, {.dim = 0, .edge = 100, .pad = sig::PadMode::None}); });
}
