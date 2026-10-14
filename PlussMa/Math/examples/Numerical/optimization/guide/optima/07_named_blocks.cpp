// Parameters can be split into named blocks of any shape, so an objective reads like the
// model it describes. Here a line y = slope * t + intercept is fitted to five points: the
// unknowns are two blocks, "slope" and "intercept", each a single number.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  using Pack = optim::ParameterPack<optim::Trace>;

  const auto t = optim::trace(vec({0.0, 1.0, 2.0, 3.0, 4.0}), false);
  const auto y = optim::trace(vec({1.1, 2.9, 5.2, 7.1, 8.8}), false);

  auto sum_squared_error = [&](const Pack& p) {
    const auto prediction = p.at("slope") * t + p.at("intercept");
    return optim::sum(optim::square(prediction - y));
  };

  const auto start = optim::pack_scalars({{"slope", pluss::make_series(0.0).to(f64)},
                                          {"intercept", pluss::make_series(0.0).to(f64)}});
  const auto result = optim::optima(sum_squared_error, start);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "slope     = " << result.params.at("slope").item<double>() << "\n";
  std::cout << "intercept = " << result.params.at("intercept").item<double>() << "\n";
  std::cout << "blocks returned: " << result.params.size() << "\n";
  return 0;
}
