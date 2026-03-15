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
  Series w = pluss::make_series(std::vector<float>{0.2f, -0.3f}, {2}, cfg);
  const Series x = pluss::make_series(std::vector<float>{2.0f, -1.0f}, {2}, cfg);
  const Series y = pluss::make_series(std::vector<float>{1.0f, 0.5f}, {2}, cfg);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 0.05;
  pluss::nn::optim::Adam optimizer(std::vector<Series*>{&w}, opt);

  for (int epoch = 1; epoch <= 80; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      auto tracked = pluss::subsp::math::diff::trace(w, true);
      const Series pred = tracked.value().mul(x);
      return pluss::nn::losses::MSE(pred, y);
    });
    if (epoch == 1 || epoch % 20 == 0) {
      std::cout << "epoch=" << epoch << " loss=" << loss.item<double>() << "\n";
    }
  }

  const auto ww = w.to_vec<float>();
  std::cout << "trained_w=[" << ww[0] << ", " << ww[1] << "]\n";
  return 0;
}
