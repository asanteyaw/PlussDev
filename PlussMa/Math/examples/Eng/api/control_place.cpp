#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const control::PlaceResult result = control::place(
      pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::make_series({0.0, 1.0}, {2, 1}),
      pluss::make_series(
          {std::complex<double>{-4.0, 0.0}, std::complex<double>{-5.0, 0.0}}));

  out.writeln("gain =", result.gain.flatten().items<double>());
  out.writeln("closed loop =", result.closed_loop.flatten().items<double>());
  out.writeln("residual =", result.residual.item<double>());
  out.writeln("condition =", result.condition.item<double>());
}
