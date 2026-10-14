// Pitfall: bounds given as a plain vector are attached to a block named "x". On a problem whose
// unknowns have other names, they do not line up, and optima refuses the problem before it
// starts, naming both layouts. The fix is to build the bounds as a pack with the same blocks.
#include <pluss/ma.h>

#include <iostream>
#include <string>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  using Pack = optim::ParameterPack<optim::Trace>;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };

  auto objective = [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")); };
  const auto start = optim::pack_scalars({{"a", scalar(1.0)}, {"b", scalar(1.0)}});

  try {
    const auto wrong = optim::constraints().lbs(pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64));
    (void)optim::optima(objective, start, {}, wrong);
  } catch (const std::exception& e) {
    std::string message = e.what();
    std::cout << "refused: " << message.substr(0, message.find('\n')) << "\n";
  }

  const auto right = optim::constraints().lbs(optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.0)}}));
  const auto result = optim::optima(objective, start, {}, right);
  std::cout << "with matching blocks: a = " << result.params.at("a").item<double>() << ", b = "
            << result.params.at("b").item<double>() << "\n";
  return 0;
}
