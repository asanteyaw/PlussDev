// When you already have the gradient, supply it directly: the objective returns the value
// together with its gradient and no autodiff is involved. The two-argument form below works
// on a plain vector; the named-block form returns the gradient as a pack with the same blocks.
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

  // f(x) = (x0 - 1)^2 + 10 (x1 + 2)^2, gradient (2 (x0 - 1), 20 (x1 + 2)).
  std::function<std::pair<pluss::Series, pluss::Series>(const pluss::Series&)> on_vector =
      [&](const pluss::Series& x) {
        const auto v = x.to_vec<double>();
        const double value = (v[0] - 1) * (v[0] - 1) + 10 * (v[1] + 2) * (v[1] + 2);
        return std::pair{pluss::make_series(value).to(f64), vec({2 * (v[0] - 1), 20 * (v[1] + 2)})};
      };
  const auto a = optim::optima(on_vector, vec({0.0, 0.0}));

  std::function<std::pair<pluss::Series, optim::ParameterPack<pluss::Series>>(const optim::ParameterPack<pluss::Series>&)>
      on_blocks = [&](const optim::ParameterPack<pluss::Series>& p) {
        const double u = p.at("u").item<double>();
        const double w = p.at("w").item<double>();
        const double value = (u - 1) * (u - 1) + 10 * (w + 2) * (w + 2);
        return std::pair{pluss::make_series(value).to(f64),
                         optim::pack_scalars({{"u", pluss::make_series(2 * (u - 1)).to(f64)},
                                              {"w", pluss::make_series(20 * (w + 2)).to(f64)}})};
      };
  const auto b = optim::optima(on_blocks, optim::pack_scalars({{"u", pluss::make_series(0.0).to(f64)},
                                                               {"w", pluss::make_series(0.0).to(f64)}}));

  const auto xa = a.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "vector form: x = [" << xa[0] << ", " << xa[1] << "]\n";
  std::cout << "block form:  u = " << b.params.at("u").item<double>() << ", w = " << b.params.at("w").item<double>() << "\n";
  return 0;
}
