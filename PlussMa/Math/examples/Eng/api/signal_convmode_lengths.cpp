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
  for (const std::int64_t n : {5, 8}) {
    for (const std::int64_t m : {3}) {
      const pluss::Series x = pluss::series::ones({n}, pluss::ScalarType::Float64);
      const pluss::Series k = pluss::series::ones({m}, pluss::ScalarType::Float64);
      out.writeln("input", n, "kernel", m, ": Full", sig::conv(x, k, -1, sig::ConvMode::Full).numel(), " Same", sig::conv(x, k, -1, sig::ConvMode::Same).numel(), " Valid", sig::conv(x, k, -1, sig::ConvMode::Valid).numel());
    }
  }
}
