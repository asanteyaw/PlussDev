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
  const pluss::Series k = pluss::make_series({0.5, 1.0, 0.25});
  const auto y = sig::conv(x, k).items<double>();
  const auto xs = x.items<double>();
  const auto ks = k.items<double>();
  double worst = 0.0;
  for (std::size_t n = 0; n < y.size(); ++n) {
    double sum = 0.0;
    for (std::size_t m = 0; m < ks.size(); ++m)
      if (n >= m && n - m < xs.size()) sum += ks[m] * xs[n - m];
    worst = std::max(worst, std::abs(sum - y[n]));
  }
  out.writeln("largest difference from the direct sum =", worst);
  const auto swapped = sig::conv(k, x).items<double>();
  double commute = 0.0;
  for (std::size_t n = 0; n < y.size(); ++n) commute = std::max(commute, std::abs(swapped[n] - y[n]));
  out.writeln("conv(x, k) against conv(k, x), largest difference =", commute);
  const pluss::Series rows = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3});
  const pluss::Series two = sig::conv(rows, pluss::make_series({1.0, 1.0}), 1);
  out.writeln("two rows convolved along the last axis, shape", two.size(0), "x", two.size(1), "first row", two.select(0, 0).items<double>());
}
