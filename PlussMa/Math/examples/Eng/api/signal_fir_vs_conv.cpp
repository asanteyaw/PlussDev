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
  const pluss::Series x = pluss::make_series({1.0, -2.0, 3.0, -4.0, 5.0});
  const pluss::Series taps = pluss::make_series({0.5, 0.3, 0.2});
  const auto causal = sig::fir(x, taps).items<double>();
  const auto full = sig::conv(x, taps).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < causal.size(); ++i) worst = std::max(worst, std::abs(causal[i] - full[i]));
  out.writeln("fir against the first", causal.size(), "samples of the full convolution, largest difference =", worst);
  out.writeln("the full convolution has", full.size(), "samples; fir keeps the causal part");
}
