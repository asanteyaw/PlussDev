#include <pluss/pluss.h>
#include <pluss/nn.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

struct TinyLSTMRegressorImpl final : pluss::nn::Module {
  pluss::nn::LSTM lstm;
  pluss::nn::Fusion head;

  explicit TinyLSTMRegressorImpl(const Config& cfg)
      : Module(true),
        lstm(pluss::nn::LSTMOptions{
            .input_size = 1,
            .hidden_size = 12,
            .num_layers = 1,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .proj_size = 0,
            .cfg = cfg,
            .seed = 11ULL,
            .training = true,
        }),
        head(12, 1, true, cfg, 17ULL) {
    module_record("lstm", lstm);
    module_record("head", head);
  }

  Series forward(const Series& x) const {
    const auto [features, state] = lstm->forward(x);
    (void)state;
    return head->forward(features);
  }
};

PLUSS_MODULE(TinyLSTMRegressor);

Series make_dataset(const Config& cfg, std::int64_t batch, std::int64_t steps) {
  std::vector<float> values;
  values.reserve(static_cast<std::size_t>(batch * steps));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < steps; ++t) {
      const float x = 0.15f * static_cast<float>(t) + 0.07f * static_cast<float>(b);
      values.push_back(std::sin(x) + 0.1f * std::cos(2.0f * x));
    }
  }
  return pluss::make_series(values, {batch, steps, 1}, cfg);
}

Series take_range(const Series& x, std::int64_t dim, std::int64_t start, std::int64_t length) {
  std::vector<std::int64_t> indices(static_cast<std::size_t>(length));
  for (std::int64_t i = 0; i < length; ++i) indices[static_cast<std::size_t>(i)] = start + i;
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  return x.index_select(dim, pluss::make_series(indices, {length}, icfg));
}

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

struct TrainStats {
  double initial_loss{0.0};
  double final_loss{0.0};
  double elapsed_seconds{0.0};
};

TrainStats train_once(pluss::Platform platform) {
  const ScalarType dtype = ScalarType::Float32;
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);

  const Series sequence = make_dataset(cfg, 24, 20);
  const Series features = take_range(sequence, 1, 0, 19);
  const Series target = take_range(sequence, 1, 1, 19);

  TinyLSTMRegressor model(cfg);
  model->train(true);
  model->to(platform, dtype);
  const auto params = model->parameters();

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 0.03;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(params, opt);

  const double initial_loss = as_f64(pluss::nn::losses::MSE(model->forward(features), target));
  const auto t0 = std::chrono::steady_clock::now();

  for (int epoch = 1; epoch <= 200; ++epoch) {
    const Series pred = model->forward(features);
    const Series loss = pluss::nn::losses::MSE(pred, target);
    optimizer.clear();
    loss.reverse();
    optimizer.step();
  }

  const auto t1 = std::chrono::steady_clock::now();
  const double final_loss = as_f64(pluss::nn::losses::MSE(model->forward(features), target));
  const std::chrono::duration<double> elapsed = t1 - t0;
  return TrainStats{
      .initial_loss = initial_loss,
      .final_loss = final_loss,
      .elapsed_seconds = elapsed.count(),
  };
}

} // namespace

int main() {
  const TrainStats cpu = train_once(pluss::pCPU);
  std::cout << "cpu dtype=f32 initial_loss=" << cpu.initial_loss << " final_loss=" << cpu.final_loss
            << " sec=" << cpu.elapsed_seconds << "\n";

  if (pluss::mps::is_ready()) {
    const TrainStats mps = train_once(pluss::pMPS);
    std::cout << "mps dtype=f32 initial_loss=" << mps.initial_loss << " final_loss=" << mps.final_loss
              << " sec=" << mps.elapsed_seconds << "\n";
    if (mps.elapsed_seconds > 0.0) {
      std::cout << "speedup_cpu_over_mps=" << (cpu.elapsed_seconds / mps.elapsed_seconds) << "\n";
    }
  } else {
    std::cout << "mps unavailable\n";
  }
  return 0;
}
