// The smallest useful call: minimize a smooth function of one vector, with no constraints.
// The objective receives the parameters as a Trace, so the solver gets exact derivatives
// without you writing any.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;

  const auto target = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}).to(f64);
  const auto start = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64);

  // f(x) = |x - target|^2, minimized where x == target.
  const auto result = optim::optima(
      [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); },
      start);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "converged: " << std::boolalpha << result.converged << "\n";
  std::cout << "exit flag: " << static_cast<int>(result.exit_flag) << "\n";
  std::cout << "iterations: " << result.iteration << "\n";
  return 0;
}
