#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::StateSpace mimo = lti::state(
      pluss::make_series({{-1.0, 0.0}, {0.0, -2.0}}),
      pluss::series::eye(2, pluss::ScalarType::Float64),
      pluss::series::eye(2, pluss::ScalarType::Float64),
      pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  try {
    static_cast<void>(control::margin(mimo, pluss::make_series({0.1, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("MIMO margin rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::margin(
        pluss::make_series({1.0, 0.5}), pluss::make_series({0.1, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("real response rejected =",
                message.substr(0, message.find('\n')));
  }
}
