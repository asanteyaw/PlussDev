#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const control::RiccatiResult result = control::care(
      pluss::make_series({-2.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({4.0}, {1, 1}),
      pluss::make_series({2.0}, {1, 1}),
      pluss::make_series({0.5}, {1, 1}));

  out.writeln("cross-weight value =", result.value.flatten().items<double>());
  out.writeln("cross-weight gain =", result.gain.flatten().items<double>());
  out.writeln("closed-loop pole =", result.closed_loop.flatten().items<double>());
}
