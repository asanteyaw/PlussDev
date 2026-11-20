// Choose portfolio weights by gradient descent: define the objective as Series arithmetic, let autodiff supply the gradient,
// and keep the weights valid by construction with a softmax.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Four assets: expected annual return and the covariance of returns.
  auto mu = pluss::make_series({0.06, 0.09, 0.12, 0.04});
  auto sigma = pluss::make_series({{0.0400, 0.0060, 0.0120, 0.0000},
                                   {0.0060, 0.0900, 0.0300, 0.0000},
                                   {0.0120, 0.0300, 0.1600, 0.0000},
                                   {0.0000, 0.0000, 0.0000, 0.0025}});
  const double risk_aversion = 3.0;

  // Parameters theta are unconstrained; the weights w = softmax(theta) are positive and sum to one.
  auto theta = pluss::series::zeros({4}, ScalarType::Float64);
  const double learning_rate = 5.0;
  for (int step = 0; step <= 2000; ++step) {
    auto param = theta.clone();
    param.tracks_slope_(true);
    auto w = param.softmax(0);
    // Maximise return minus a penalty on variance: minimise the negative.
    auto expected = (w * mu).sum();
    auto variance = w.unsqueeze(0).matmul(sigma).matmul(w.unsqueeze(1)).squeeze(1).squeeze(0);
    auto loss = variance * (0.5 * risk_aversion) - expected;
    loss.reverse();
    if (step % 400 == 0)
      std::cout << "step " << step << "  objective " << -loss.item<double>() << "  return " << expected.item<double>() << "  variance " << variance.item<double>() << "\n";
    theta = param.detach() - param.sensitivity() * learning_rate;
  }
  auto weights = theta.softmax(0);
  show("optimal weights", weights);
  std::cout << "weights sum to " << weights.sum().item<double>() << "\n";

  // Check: at a constrained optimum the marginal utility mu - lambda Sigma w is equal across every asset that is held.
  auto marginal = mu - sigma.matmul(weights.unsqueeze(1)).squeeze(1) * risk_aversion;
  show("marginal utility per asset", marginal);
}
