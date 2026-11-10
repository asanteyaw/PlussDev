#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series input =
      pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  const pluss::Series numerator = pluss::make_series({0.5, 0.5});
  const pluss::Series denominator = pluss::make_series({1.0, -0.5});
  const sig::FilterRun first =
      sig::filter(input.slice(0, 0, 4), numerator, denominator);
  const sig::FilterRun second = sig::filter(
      input.slice(0, 4, 8), numerator, denominator, first.state);
  const pluss::Series joined = first.values.unite({second.values}, 0);

  out.writeln("first block =", first.values.items<double>());
  out.writeln("carried state =", first.state.items<double>());
  out.writeln("second block =", second.values.items<double>());
  out.writeln("joined output =", joined.items<double>());

  const pluss::Series smooth = sig::zero_phase(
      input, numerator, denominator,
      {.dim = 0, .edge = 3, .pad = sig::PadMode::Odd});
  out.writeln("zero phase =", smooth.items<double>());
}
