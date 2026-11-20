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
  const pluss::Series x = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const pluss::Series b = pluss::make_series({1.0});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  const auto y = sig::filter(x, b, a).values.items<double>();
  out.writeln("impulse response of y[n] = x[n] + 0.5 y[n-1] =", y);
  double worst = 0.0, expected = 1.0;
  for (std::size_t n = 0; n < y.size(); ++n, expected *= 0.5) worst = std::max(worst, std::abs(y[n] - expected));
  out.writeln("largest difference from 0.5^n =", worst);
}
