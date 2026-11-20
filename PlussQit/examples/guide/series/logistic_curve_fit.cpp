// Fit a nonlinear growth curve to noisy observations with the Adam optimiser written from Series operations.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Observations of a logistic curve  y = K / (1 + exp(-r (t - t0)))  with K = 50, r = 0.8, t0 = 6 and a little noise.
  const std::int64_t n = 40;
  auto t = pluss::series::linspace(0.0, 12.0, n, ScalarType::Float64);
  auto truth = (((t - 6.0) * -0.8).exp() + 1.0).reciprocal() * 50.0;
  auto y = truth + pluss::series::randn({n}, ScalarType::Float64, 31) * 1.0;

  // Parameters (K, r, t0) start away from the answer. Adam keeps a running mean of the gradient and of its square.
  auto p = pluss::make_series({30.0, 0.3, 4.0});
  auto m = pluss::series::zeros({3}, ScalarType::Float64);
  auto v = pluss::series::zeros({3}, ScalarType::Float64);
  const double lr = 0.1, beta1 = 0.9, beta2 = 0.999, eps = 1e-8;
  for (int step = 1; step <= 600; ++step) {
    auto param = p.clone();
    param.tracks_slope_(true);
    auto curve = (((t - param.select(0, 2)) * -param.select(0, 1)).exp() + 1.0).reciprocal() * param.select(0, 0);
    auto loss = (curve - y).square().mean();
    loss.reverse();
    auto g = param.sensitivity();
    m = m * beta1 + g * (1.0 - beta1);
    v = v * beta2 + g.square() * (1.0 - beta2);
    auto m_hat = m / (1.0 - std::pow(beta1, step));
    auto v_hat = v / (1.0 - std::pow(beta2, step));
    p = param.detach() - m_hat / (v_hat.sqrt() + eps) * lr;
    if (step == 1 || step % 100 == 0) std::cout << "step " << step << "  loss " << loss.item<double>() << "\n";
  }
  show("fitted (K, r, t0)", p);
  std::cout << "true   (K, r, t0): 50, 0.8, 6\n";
  auto fitted = (((t - p.select(0, 2)) * -p.select(0, 1)).exp() + 1.0).reciprocal() * p.select(0, 0);
  std::cout << "residual standard deviation " << (fitted - y).std().item<double>() << " (noise 1.0)\n";
}
