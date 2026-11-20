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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  const pluss::Series k = pluss::make_series({1.0, 2.0, 4.0});
  out.writeln("corr(x, k)         =", sig::corr(x, k).items<double>());
  out.writeln("conv(x, flip(k))   =", sig::conv(x, k.flip(0)).items<double>());
  out.writeln("conv(x, k)         =", sig::conv(x, k).items<double>());
}
