// Pitfall: the objective must return one number. Returning a vector of residuals instead of
// their sum of squares is a common slip. optima does not throw; it returns immediately with the
// InvalidProblem exit flag and a message that says what to change.
#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto start = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}).to(pluss::ScalarType::Float64);

  const auto result = optim::optima([](const pluss::Series& x) { return x.square(); }, start);

  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::string message = result.message;
  std::cout << "message: " << message.substr(0, message.find('\n')) << "\n";
  return 0;
}
