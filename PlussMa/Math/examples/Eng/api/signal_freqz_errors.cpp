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
  const pluss::Series omega = pluss::make_series({0.0, 1.0});
  refused(out, "an integer numerator", [&] { sig::freqz(pluss::make_series({1, 2}), omega); });
  refused(out, "an empty numerator", [&] { sig::freqz(pluss::series::zeros({0}, pluss::ScalarType::Float64), omega); });
  refused(out, "a scalar frequency grid", [&] { sig::freqz(pluss::make_series({1.0, 1.0}), pluss::make_series(0.5)); });
}
