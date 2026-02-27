#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using namespace pluss;
  using namespace pluss::subsp::math::optim;

  const Series x = pluss::linspace(-1.0, 1.0, 256, ScalarType::Float64);
  const Series y = x.mul(2.0).sub(0.5);

  std::vector<Series> params{
      pluss::make_series(0.0).to(ScalarType::Float64),
      pluss::make_series(0.0).to(ScalarType::Float64),
  };

  AdamState state = adam_state_like(params);
  AdamOptions opt;
  opt.learning_rate = 0.08;

  for (std::int64_t step = 1; step <= 300; ++step) {
    auto [loss, sensitivities] = value_and_sensitivity(
        [&](const std::vector<Trace>& p) -> Trace {
          const Trace w = p[0];
          const Trace b = p[1];
          const Trace vx = trace(x, false);
          const Trace vy = trace(y, false);
          const Trace pred = w * vx + b;
          const Trace err = pred - vy;
          return mean(err * err);
        },
        params);

    adam_step_inplace(params, sensitivities, state, opt);

    if (step % 50 == 0 || step == 1) {
      std::cout << "step=" << step << " loss=" << loss.item<double>() << '\n';
    }
  }

  std::cout << "learned w=" << params[0].item<double>() << " b=" << params[1].item<double>() << '\n';

  // Nonlinear autograd smoke: exercise recent tape rules in one scalar objective.
  std::vector<Series> probe_params{
      pluss::make_series(0.7).to(ScalarType::Float64),
  };
  auto [probe_loss, probe_sens] = value_and_sensitivity(
      [&](const std::vector<Trace>& p) -> Trace {
        const Trace z = p[0];
        const Trace y0 = square(z);
        const Trace y1 = log(exp(z) + 1.0);
        const Trace y2 = sqrt(y0 + 2.0);
        const Trace y3 = tanh(z) + sin(z) - cos(z);
        const Trace y4 = relu(z - 0.2) + clamp(z, -1.0, 1.0);
        const Trace y5 = pow(z, 3.0) + rsqrt(y0 + 1.0);
        const Trace y6 = softplus(z, 1.5, 18.0) + sigmoid(z);
        return y1 + y2 + y3 + y4 + y5 + y6;
      },
      probe_params);

  const double probe_loss_v = probe_loss.item<double>();
  const double probe_sens_v = probe_sens[0].item<double>();
  std::cout << "probe loss=" << probe_loss_v << " sensitivity=" << probe_sens_v << '\n';
  if (!std::isfinite(probe_loss_v) || !std::isfinite(probe_sens_v)) {
    std::cerr << "nonlinear autograd smoke failed: non-finite value/sensitivity\n";
    return 2;
  }
  return 0;
}
