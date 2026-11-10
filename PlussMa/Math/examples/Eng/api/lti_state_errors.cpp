#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(lti::state(
        pluss::make_series({{1.0, 0.0}, {0.0, 1.0}}),
        pluss::make_series({1.0, 0.0}, {2, 1}),
        pluss::make_series({{1.0, 0.0}}),
        pluss::make_series({{0.0, 0.0}})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("bad feedthrough rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::state(
        pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}), -0.1));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative sample time rejected =",
                message.substr(0, message.find('\n')));
  }
}
