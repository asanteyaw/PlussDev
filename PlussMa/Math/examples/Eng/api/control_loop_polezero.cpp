#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::PoleZero plant = lti::zpk(
      pluss::make_series(std::initializer_list<std::complex<double>>{}),
      pluss::make_series({std::complex<double>{-1.0, 0.0}}),
      pluss::make_series(2.0));
  const control::LoopResult result =
      control::loop(plant, pluss::make_series({0.0, 1.0}));

  out.writeln("pole-zero loop =",
              result.response.items<std::complex<double>>());
  out.writeln("S plus T =",
              (result.sens + result.comp).items<std::complex<double>>());
}
