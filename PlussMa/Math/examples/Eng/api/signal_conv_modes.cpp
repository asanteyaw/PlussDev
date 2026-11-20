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
  const pluss::Series k = pluss::make_series({1.0, 0.0, -1.0});
  for (const auto [label, mode] : {std::pair{"full ", sig::ConvMode::Full}, {"same ", sig::ConvMode::Same}, {"valid", sig::ConvMode::Valid}}) {
    const pluss::Series y = sig::conv(x, k, -1, mode);
    out.writeln(label, "length", y.numel(), "values", y.items<double>());
  }
}
