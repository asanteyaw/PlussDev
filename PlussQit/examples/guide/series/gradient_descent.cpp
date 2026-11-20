// Train a model by gradient descent: let the library differentiate a loss, step the parameters downhill and watch the loss fall.
#include <pluss/pluss.h>

#include <iostream>

int main() {
  using pluss::ScalarType;
  // Data from a known line y = 4 - 1.5 x plus a little noise.
  const std::int64_t n = 100;
  auto x = pluss::series::linspace(-2.0, 2.0, n, ScalarType::Float64);
  auto y = x * -1.5 + 4.0 + pluss::series::randn({n}, ScalarType::Float64, 3) * 0.2;

  auto w = pluss::make_series({0.0, 0.0});   // parameters: intercept, slope
  const double learning_rate = 0.1;
  for (int step = 0; step <= 100; ++step) {
    // 1. Track the parameters so the loss can be differentiated with respect to them.
    auto param = w.to(ScalarType::Float64);
    param.tracks_slope_(true);
    // 2. Forward pass and mean squared error.
    auto prediction = param.select(0, 0) + param.select(0, 1) * x;
    auto loss = (prediction - y).square().mean();
    // 3. Reverse pass: the sensitivity of the loss to each parameter is its gradient.
    loss.reverse();
    auto gradient = param.sensitivity();
    if (step % 20 == 0)
      std::cout << "step " << step << "  loss " << loss.item<double>() << "  intercept " << param.select(0, 0).item<double>()
                << "  slope " << param.select(0, 1).item<double>() << "\n";
    // 4. Step against the gradient; the new parameters start a fresh graph on the next pass.
    w = param.detach() - gradient * learning_rate;
  }
  auto closed = x.unsqueeze(1);
  std::cout << "least-squares answer for comparison: ";
  auto design = pluss::series::ones({n, 1}, ScalarType::Float64).unite({closed}, 1);
  auto beta = design.t().matmul(design).linearalg().inverse().matmul(design.t().matmul(y.unsqueeze(1)));
  auto coefficients = beta.squeeze(1);
  std::cout << coefficients.select(0, 0).item<double>() << ", " << coefficients.select(0, 1).item<double>() << "\n";
}
