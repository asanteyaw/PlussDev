#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  // The spectrum of a 64-point window: the width of its main lobe and the level of its highest sidelobe, relative to the gain at zero frequency.
  const std::int64_t n = 64;
  const std::int64_t points = 8192;
  std::vector<double> grid(points);
  for (std::int64_t i = 0; i < points; ++i) grid[i] = std::acos(-1.0) * i / (points - 1);
  const pluss::Series w = sig::hamming(n);
  const auto h = sig::freqz(w, pluss::make_series(grid)).abs().items<double>();
  std::int64_t null = 1;
  while (null + 1 < points && h[null + 1] < h[null]) ++null;   // the first minimum ends the main lobe
  double sidelobe = 0.0;
  for (std::int64_t i = null; i < points; ++i) sidelobe = std::max(sidelobe, h[i]);
  out.writeln("main lobe ends at", grid[null] * n / (2.0 * std::acos(-1.0)), "bins from zero frequency; highest sidelobe", 20.0 * std::log10(sidelobe / h[0]), "dB below the main lobe");
}
