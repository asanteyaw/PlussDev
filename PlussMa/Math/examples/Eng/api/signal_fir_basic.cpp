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
  const pluss::Series x = pluss::make_series({3.0, 6.0, 9.0, 12.0, 15.0});
  const pluss::Series taps = pluss::make_series({1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0});
  const pluss::Series y = sig::fir(x, taps);
  out.writeln("input length", x.numel(), " output length", y.numel());
  out.writeln("three-point moving average =", y.items<double>());
}
