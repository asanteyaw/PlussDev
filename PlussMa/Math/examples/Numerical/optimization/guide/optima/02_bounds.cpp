// Bounds restrict each coordinate to an interval. The unconstrained minimum below sits at
// (3, -2); with 0 <= x <= 2 and 0 <= y <= 2 the answer is the nearest corner of the box.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({3.0, -2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto box = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({2.0, 2.0}));
  const auto result = optim::optima(objective, vec({1.0, 1.0}), {}, box);

  const auto x = result.params.at("x").to_vec<double>();
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "objective = " << result.evaluation.value.item<double>() << "\n";
  std::cout << "max violation = " << result.evaluation.max_constraint_violation << "\n";
  return 0;
}
