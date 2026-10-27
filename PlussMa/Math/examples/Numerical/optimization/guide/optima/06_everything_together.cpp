// Every constraint family can be combined in one builder chain: bounds, linear equality and
// inequality, and nonlinear equality and inequality. The unknowns are three named scalar
// blocks a, b, c. Bounds are given as packs with the same block names; linear rows address
// the blocks in order, one column per scalar.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };
  using Pack = optim::ParameterPack<pluss::Series>;

  // Minimize a^2 + 2 b^2 + 3 c^2 - 4 a.
  auto objective = [](const Pack& p) {
    return p.at("a").square() + 2.0 * p.at("b").square() + 3.0 * p.at("c").square() -
           4.0 * p.at("a");
  };

  const auto constraints =
      optim::constraints()
          .lbs(optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}, {"c", scalar(0.0)}}))
          .ubs(optim::pack_scalars({{"a", scalar(1.5)}, {"b", scalar(2.0)}, {"c", scalar(2.0)}}))
          .ln_eq(mat({0, 1, 1}, 1, 3), vec({1.0}))                         // b + c = 1
          .ln_ineq(mat({1, 1, 1}, 1, 3), vec({2.5}))                       // a + b + c <= 2.5
          .nl_eq([](const Pack& p) { return p.at("a") * p.at("b") - 0.3; })  // a * b = 0.3
          .nl_ineq([](const Pack& p) { return p.at("c").square() - 0.6; });  // c^2 <= 0.6

  const auto start = optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.5)}, {"c", scalar(0.5)}});
  const auto result = optim::optima(objective, start, {}, constraints);

  const double a = as_f64(result.params.at("a"));
  const double b = as_f64(result.params.at("b"));
  const double c = as_f64(result.params.at("c"));
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "a = " << a << ", b = " << b << ", c = " << c << "\n";
  std::cout << "b + c = " << b + c << " (target 1)\n";
  std::cout << "a * b = " << a * b << " (target 0.3)\n";
  std::cout << "c^2 = " << c * c << " (limit 0.6)\n";
  std::cout << "max violation below 1e-5: " << std::boolalpha << (result.evaluation.max_constraint_violation <= 1e-5)
            << "\n";
  return 0;
}
