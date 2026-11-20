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
  const pluss::Series pattern = pluss::make_series({0.0, 1.0, 3.0, 1.0, 0.0});
  const pluss::Series signal = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 3.0, 1.0, 0.0, 0.0, 0.0});
  const pluss::Series c = sig::corr(signal, pattern, -1, sig::ConvMode::Valid);
  const auto values = c.items<double>();
  const auto peak = std::max_element(values.begin(), values.end()) - values.begin();
  out.writeln("correlation values =", values);
  out.writeln("the pattern starts at sample", peak, "of the signal, where the correlation peaks at", values[peak]);
}
