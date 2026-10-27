// The same constrained problem solved by each of the four algorithms. They share the result type,
// so the answer, the work done and the stopping message can be compared directly.
// Problem: minimize (x0 - 2)^2 + (x1 - 1)^2 subject to x0 <= 1.5 and x1 <= 0.8.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };
  const auto constraints = optim::constraints().ubs(vec({1.5, 0.8}));

  const std::vector<std::pair<std::string, optim::OptimaAlgorithm>> algorithms = {
      {"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
      {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::fixed << std::setprecision(4);
  for (const auto& [name, algorithm] : algorithms) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(objective, vec({0.5, 0.5}), options, constraints);
    const auto x = r.params.at("x").host<double>();
    std::cout << std::left << std::setw(17) << name << std::setw(24) << ("label=" + r.output.algorithm) << std::right
              << "x=[" << x[0] << ", " << x[1] << "]  flag=" << std::setw(2) << static_cast<int>(r.exit_flag)
              << "  iterations=" << std::setw(3) << r.iteration << "  evaluations=" << std::setw(3)
              << r.function_evaluations << "\n";
  }
  return 0;
}
