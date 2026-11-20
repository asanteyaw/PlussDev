#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  // The symmetric window is its own mirror image; the periodic one is not, because its last coefficient is dropped.
  const std::int64_t n = 8;
  for (const auto mode : {sig::WindowMode::Symmetric, sig::WindowMode::Periodic}) {
    const auto w = sig::hann(n, {.mode = mode}).items<double>();
    double asymmetry = 0.0;
    for (std::int64_t i = 0; i < n; ++i) asymmetry = std::max(asymmetry, std::abs(w[i] - w[n - 1 - i]));
    out.writeln(mode == sig::WindowMode::Symmetric ? "symmetric" : "periodic ", ": largest |w[i] - w[n-1-i]| =", asymmetry, " peak =", *std::max_element(w.begin(), w.end()));
  }
}
