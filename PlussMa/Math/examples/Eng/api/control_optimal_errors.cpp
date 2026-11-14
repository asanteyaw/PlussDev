#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace control = pluss::subsp::eng::control;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const pluss::Series scalar = pluss::make_series({1.0}, {1, 1});
  try {
    static_cast<void>(control::care(
        pluss::make_series({-2.0}, {1, 1}), scalar,
        pluss::make_series({4.0}, {1, 1}),
        pluss::make_series({0.0}, {1, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("singular R rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(control::care(
        pluss::make_series({{-1.0, 0.0}, {0.0, -2.0}}),
        pluss::make_series({1.0, 0.0}, {2, 1}),
        pluss::make_series({{1.0, 2.0}, {0.0, 1.0}}), scalar));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("non-Hermitian Q rejected =",
                message.substr(0, message.find('\n')));
  }
}
