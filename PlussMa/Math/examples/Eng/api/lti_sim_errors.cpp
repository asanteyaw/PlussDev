#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::StateSpace system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  try {
    static_cast<void>(lti::sim(
        system, pluss::make_series({1.0, 1.0, 1.0}, {3, 1}),
        pluss::make_series({0.0, 0.5, 0.5})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("repeated time rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::sim(
        system, pluss::make_series({1.0, 1.0, 1.0}),
        pluss::make_series({0.0, 0.5, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("rank-one input rejected =", message.substr(0, message.find('\n')));
  }
}
