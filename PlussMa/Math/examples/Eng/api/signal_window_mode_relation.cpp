#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  // A periodic window of length n is the first n coefficients of the symmetric window of length n + 1.
  const std::int64_t n = 16;
  using Maker = pluss::Series (*)(std::int64_t, const sig::WindowOptions &);
  const std::pair<const char *, Maker> makers[] = {{"boxcar", sig::boxcar}, {"bartlett", sig::bartlett}, {"blackman", sig::blackman}, {"hamming", sig::hamming}, {"hann", sig::hann}};
  for (const auto &[name, make] : makers) {
    const auto periodic = make(n, {.mode = sig::WindowMode::Periodic}).items<double>();
    const auto longer = make(n + 1, {}).items<double>();
    double worst = 0.0;
    for (std::int64_t i = 0; i < n; ++i) worst = std::max(worst, std::abs(periodic[i] - longer[i]));
    out.writeln(name, ": periodic(16) vs symmetric(17), largest difference =", worst);
  }
}
