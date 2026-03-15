#include <pluss/pluss.h>

#include <iostream>

int main() {
  using pluss::ScalarType;
  using pluss::subsp::math::diff::Trace;
  using pluss::subsp::math::diff::trace;
  using pluss::subsp::math::diff::value_and_grad;

  const auto x = pluss::make_series(1.25).to(ScalarType::Float64);

  auto tracked_x = trace(x, true);
  auto tracked_y = tracked_x * tracked_x * tracked_x - 2.0 * (tracked_x * tracked_x) + tracked_x;
  tracked_y.reverse();

  std::cout << "reverse value: " << tracked_y.value().item<double>() << "\n";
  std::cout << "reverse grad : " << tracked_x.sensitivity().item<double>() << "\n";

  const auto [value, grad] = value_and_grad(
      [](const Trace& z) -> Trace {
        return z * z * z - 2.0 * (z * z) + z;
      },
      x);

  std::cout << "value_and_grad value: " << value.item<double>() << "\n";
  std::cout << "value_and_grad grad : " << grad.item<double>() << "\n";

  return 0;
}
