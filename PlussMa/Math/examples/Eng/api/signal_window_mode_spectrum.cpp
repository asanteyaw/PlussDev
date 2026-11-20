#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  // A periodic window tiles: summed over shifts by n / 2 it is constant, which keeps the overlap-add of a half-overlapped analysis flat.
  const std::int64_t n = 16;
  for (const auto mode : {sig::WindowMode::Symmetric, sig::WindowMode::Periodic}) {
    const auto w = sig::hann(n, {.mode = mode}).items<double>();
    double lowest = 1e9, highest = -1e9;
    for (std::int64_t i = 0; i < n / 2; ++i) {
      const double total = w[i] + w[i + n / 2];
      lowest = std::min(lowest, total);
      highest = std::max(highest, total);
    }
    out.writeln(mode == sig::WindowMode::Symmetric ? "symmetric" : "periodic ", ": overlap-add of two half-shifted windows spans", lowest, "to", highest);
  }
}
