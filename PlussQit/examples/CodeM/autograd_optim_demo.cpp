#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using namespace pluss;
  using namespace pluss::subsp::math::optim;

  const Series x = pluss::linspace(-1.0, 1.0, 256, ScalarType::Float64);
  const Series y = x.mul(2.0).sub(0.5);

  std::vector<Series> params{
      pluss::value(0.0).to(ScalarType::Float64),
      pluss::value(0.0).to(ScalarType::Float64),
  };

  AdamState state = adam_state_like(params);
  AdamOptions opt;
  opt.learning_rate = 0.08;

  for (std::int64_t step = 1; step <= 300; ++step) {
    auto [loss, grads] = value_and_grad(
        [&](const std::vector<Variable>& p) -> Variable {
          const Variable w = p[0];
          const Variable b = p[1];
          const Variable vx = variable(x, false);
          const Variable vy = variable(y, false);
          const Variable pred = w * vx + b;
          const Variable err = pred - vy;
          return mean(err * err);
        },
        params);

    adam_step_inplace(params, grads, state, opt);

    if (step % 50 == 0 || step == 1) {
      std::cout << "step=" << step << " loss=" << loss.item<double>() << '\n';
    }
  }

  std::cout << "learned w=" << params[0].item<double>() << " b=" << params[1].item<double>() << '\n';
  return 0;
}
