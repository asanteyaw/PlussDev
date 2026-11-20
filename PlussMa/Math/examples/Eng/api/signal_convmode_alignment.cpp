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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  const pluss::Series k = pluss::make_series({1.0, 1.0, 1.0});
  const auto full = sig::conv(x, k, -1, sig::ConvMode::Full).items<double>();
  const auto same = sig::conv(x, k, -1, sig::ConvMode::Same).items<double>();
  const auto valid = sig::conv(x, k, -1, sig::ConvMode::Valid).items<double>();
  out.writeln("full  =", full);
  out.writeln("same  =", same, " (the centre of full)");
  out.writeln("valid =", valid, " (only where the kernel fits entirely)");
}
