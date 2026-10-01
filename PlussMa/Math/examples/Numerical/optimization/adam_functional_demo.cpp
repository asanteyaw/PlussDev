#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  Series params = pluss::make_series(std::vector<double>{1.0, -2.0}, {2}, cfg);
  Series params_tape = pluss::make_series(std::vector<double>{1.0, -2.0}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<double>{3.0, 3.0}, {2}, cfg);

  pluss::optim::AdamOptions options{};
  options.learning_rate = 0.1;

  pluss::optim::AdamState state = pluss::optim::adam_state_like(std::vector<Series>{params});
  pluss::optim::AdamState state_tape = pluss::optim::adam_state_like(std::vector<Series>{params_tape});

  std::cout << "[functional] explicit gradients\n";
  for (int iter = 1; iter <= 60; ++iter) {
    // f(x) = 0.5 * ||x - target||^2 -> grad f = x - target
    const Series grad = params.sub(target);
    std::vector<Series> step_params{params};
    pluss::optim::adam_step_inplace(step_params, std::vector<Series>{grad}, state, options);
    params = step_params[0];

    if (iter % 15 == 0) {
      const double loss = params.sub(target).square().sum().mul(0.5).item<double>();
      std::cout << "iter=" << iter << " loss=" << loss << '\n';
    }
  }

  const double p0 = params.select(0, 0).item<double>();
  const double p1 = params.select(0, 1).item<double>();
  std::cout << "final params explicit: [" << p0 << ", " << p1 << "]\n";

  std::cout << "[functional] autodiff/tape gradients\n";
  for (int iter = 1; iter <= 60; ++iter) {
    auto tracked = pluss::subsp::math::diff::trace(params_tape, true);
    const Series loss = tracked.value().sub(target).square().sum().mul(0.5);
    tracked.clear();
    loss.reverse();

    const Series grad = params_tape.sensitivity();
    std::vector<Series> step_params{params_tape};
    pluss::optim::adam_step_inplace(step_params, std::vector<Series>{grad}, state_tape, options);
    params_tape = step_params[0];

    if (iter % 15 == 0) {
      std::cout << "iter=" << iter << " loss=" << loss.item<double>() << '\n';
    }
  }

  const double t0 = params_tape.select(0, 0).item<double>();
  const double t1 = params_tape.select(0, 1).item<double>();
  std::cout << "final params autodiff: [" << t0 << ", " << t1 << "]\n";
  return 0;
}
