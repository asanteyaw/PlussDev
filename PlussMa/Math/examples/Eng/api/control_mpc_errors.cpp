#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const control::mpc::Weights weights{
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.1}, {1, 1})};
  const control::mpc::Bounds bounds{
      pluss::make_series({-0.5}), pluss::make_series({0.5})};
  try {
    const lti::StateSpace continuous = lti::state(
        pluss::make_series({-1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1}));
    static_cast<void>(control::mpc::build(continuous, weights, bounds));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("continuous plant rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    const lti::StateSpace discrete = lti::state(
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1}), 1.0);
    static_cast<void>(control::mpc::build(
        discrete, weights, bounds, {.horizon = 0}));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("zero horizon rejected =", message.substr(0, message.find('\n')));
  }
}
