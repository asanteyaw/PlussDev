#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // The mean is the coherent gain (the amplitude a window leaves a steady tone) and the mean square is the power it leaves in noise.
  for (const std::int64_t n : {8, 64, 512}) {
    const pluss::Series w = sig::bartlett(n);
    out.writeln("n =", n, " coherent gain =", w.mean().item<double>(), " mean square =", (w * w).mean().item<double>(), " peak =", w.max().item<double>());
  }
}
