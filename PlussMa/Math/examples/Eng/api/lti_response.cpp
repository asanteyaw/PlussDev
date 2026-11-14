#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  const lti::StateSpace state_system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const lti::PoleZero root_system = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({-1.0}),
      pluss::make_series(1.0));
  const pluss::Series omega = pluss::make_series({0.0, 1.0, 2.0});

  out.writeln("state response =",
              lti::freqresp(state_system, omega).flatten().items<std::complex<double>>());
  out.writeln("root response =",
              lti::freqresp(root_system, omega).items<std::complex<double>>());
}
