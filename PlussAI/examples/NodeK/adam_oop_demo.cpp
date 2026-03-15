#include <pluss/pluss.h>
#include <pluss/nn.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  Series params = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
  Series params_tape = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<float>{3.0f, 3.0f}, {2}, cfg);

  pluss::nn::optim::AdamOptions options{};
  options.learning_rate = 0.1;

  pluss::nn::optim::Adam optimizer(std::vector<Series*>{&params}, options);
  pluss::nn::optim::Adam optimizer_tape(std::vector<Series*>{&params_tape}, options);

  std::cout << "[oop] explicit gradients\n";
  for (int iter = 1; iter <= 60; ++iter) {
    // f(x) = 0.5 * ||x - target||^2 -> grad f = x - target
    const Series grad = params - target;
    optimizer.step(std::vector<Series>{grad});

    if (iter % 15 == 0) {
      const double loss = params.sub(target).square().sum().mul(0.5).item<double>();
      std::cout << "iter=" << iter << " loss=" << loss << '\n';
    }
  }

  const double p0 = params.select(0, 0).item<double>();
  const double p1 = params.select(0, 1).item<double>();
  std::cout << "final params explicit: [" << p0 << ", " << p1 << "]\n";

  std::cout << "[oop] autodiff/tape gradients via optimizer.step()\n";
  for (int iter = 1; iter <= 60; ++iter) {
    const Series loss = optimizer_tape.step([&]() -> Series {
      auto tracked = pluss::subsp::math::diff::trace(params_tape, true);
      return tracked.value().sub(target).square().sum().mul(0.5);
    });

    if (iter % 15 == 0) {
      std::cout << "iter=" << iter << " loss=" << loss.item<double>() << '\n';
    }
  }

  const double t0 = params_tape.select(0, 0).item<double>();
  const double t1 = params_tape.select(0, 1).item<double>();
  std::cout << "final params autodiff: [" << t0 << ", " << t1 << "]\n";
  return 0;
}
