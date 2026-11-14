#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  const lti::PoleZero plant = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(1.0));
  const pluss::Series omega = pluss::series::logspace(
      -3.0, 3.0, 20001, 10.0,
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const control::MarginResult result = control::margin(plant, omega);

  out.writeln("gain margin =", result.gain.item<double>());
  out.writeln("gain frequency =", result.gain_freq.item<double>());
  out.writeln("phase margin =", result.phase.item<double>());
  out.writeln("phase frequency =", result.phase_freq.item<double>());
  out.writeln("stability margin =", result.stability.item<double>());
}
