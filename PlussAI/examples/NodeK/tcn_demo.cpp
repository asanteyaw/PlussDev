#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Series make_time_series_batch(const Config& cfg) {
  std::vector<double> values;
  values.reserve(8 * 24);
  for (std::int64_t batch = 0; batch < 8; ++batch) {
    for (std::int64_t step = 0; step < 24; ++step) {
      const double t = 0.22 * static_cast<double>(step) + 0.11 * static_cast<double>(batch);
      const double base = 0.7 * std::sin(t) + 0.2 * std::cos(2.0 * t);
      const double trend = 0.015 * static_cast<double>(step);
      values.push_back(base + trend);
    }
  }
  return pluss::make_series(values, {8, 1, 24}, cfg);
}

Series take_range(const Series& x, std::int64_t dim, std::int64_t start, std::int64_t length) {
  std::vector<std::int64_t> indices(static_cast<std::size_t>(length));
  for (std::int64_t i = 0; i < length; ++i) {
    indices[static_cast<std::size_t>(i)] = start + i;
  }
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  return x.index_select(dim, pluss::make_series(indices, {length}, icfg));
}

Series causal_tcn_block(const Series& input,
                        const pluss::nn::Conv1d& conv_a,
                        const pluss::nn::Conv1d& conv_b,
                        const pluss::nn::Conv1d* residual_proj = nullptr) {
  Series residual = input;
  if (residual_proj != nullptr) {
    residual = residual_proj->forward(input);
  }
  const Series hidden = conv_a->forward(input).relu();
  const Series updated = conv_b->forward(hidden).relu();
  return updated.add(residual);
}

} // namespace

int main() {
  using namespace pluss;

  const Config cfg = f64();
  const Series series = make_time_series_batch(cfg);
  const Series features = take_range(series, 2, 0, 23);
  const Series target = take_range(series, 2, 1, 23);

  // A torch-like strictly causal TCN stack: left-only padding with dilations 1, 2, 4.
  // causal_padding1d(kernel, dilation) returns {left_pad, 0}, so no future timestep leaks in.
  nn::Conv1d input_proj(nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 8,
      .kernel_size = 1,
      .stride = 1,
      .padding = 0,
      .padding_lr = pluss::nn::causal_padding1d(1, 1),
      .dilation = 1,
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 101ULL,
  });
  nn::Conv1d block1_a(pluss::nn::Conv1dOptions{.in_channels = 8, .out_channels = 8, .kernel_size = 3, .stride = 1, .padding = 0, .padding_lr = pluss::nn::causal_padding1d(3, 1), .dilation = 1, .groups = 1, .bias = true, .cfg = cfg, .seed = 103ULL});
  nn::Conv1d block1_b(pluss::nn::Conv1dOptions{.in_channels = 8, .out_channels = 8, .kernel_size = 3, .stride = 1, .padding = 0, .padding_lr = pluss::nn::causal_padding1d(3, 1), .dilation = 1, .groups = 1, .bias = true, .cfg = cfg, .seed = 107ULL});
  nn::Conv1d block2_a(pluss::nn::Conv1dOptions{.in_channels = 8, .out_channels = 8, .kernel_size = 3, .stride = 1, .padding = 0, .padding_lr = pluss::nn::causal_padding1d(3, 2), .dilation = 2, .groups = 1, .bias = true, .cfg = cfg, .seed = 109ULL});
  nn::Conv1d block2_b(pluss::nn::Conv1dOptions{.in_channels = 8, .out_channels = 8, .kernel_size = 3, .stride = 1, .padding = 0, .padding_lr = pluss::nn::causal_padding1d(3, 2), .dilation = 2, .groups = 1, .bias = true, .cfg = cfg, .seed = 113ULL});
  nn::Conv1d block3_a(pluss::nn::Conv1dOptions{.in_channels = 8, .out_channels = 8, .kernel_size = 3, .stride = 1, .padding = 0, .padding_lr = pluss::nn::causal_padding1d(3, 4), .dilation = 4, .groups = 1, .bias = true, .cfg = cfg, .seed = 127ULL});
  nn::Conv1d block3_b(pluss::nn::Conv1dOptions{.in_channels = 8, .out_channels = 8, .kernel_size = 3, .stride = 1, .padding = 0, .padding_lr = pluss::nn::causal_padding1d(3, 4), .dilation = 4, .groups = 1, .bias = true, .cfg = cfg, .seed = 131ULL});
  nn::Fusion head(8, 1, true, cfg, 137ULL);

  const Series stem = input_proj->forward(features).relu();
  const Series block1 = causal_tcn_block(stem, block1_a, block1_b);
  const Series block2 = causal_tcn_block(block1, block2_a, block2_b);
  const Series block3 = causal_tcn_block(block2, block3_a, block3_b);
  const Series prediction = head->forward(block3.transpose(1, 2)).transpose(1, 2);

  std::cout << "tcn_demo\n";
  std::cout << "surface: pluss::nn::Conv1d + pluss::nn::Fusion via #include <pluss/pluss.h> + <pluss/ai.h>\n";
  std::cout << "task: next-step time-series forecasting with a strictly causal, dilated torch-like TCN stack\n";
  std::cout << "user-defined hyperparameters: kernel_size, stride, padding or padding_lr, dilation, groups, bias, channels, depth\n";
  std::cout << "dilation schedule used here: [1, 2, 4]\n";
  std::cout << "causal padding helper: pluss::nn::causal_padding1d(kernel_size, dilation)\n";
  std::cout << "input shape: " << features.sizes()[0] << " x " << features.sizes()[1] << " x " << features.sizes()[2] << "\n";
  std::cout << "latent shape after block 3: " << block3.sizes()[0] << " x " << block3.sizes()[1] << " x " << block3.sizes()[2] << "\n";
  std::cout << "prediction shape: " << prediction.sizes()[0] << " x " << prediction.sizes()[1] << " x " << prediction.sizes()[2] << "\n";
  std::cout << "initial mse: " << pluss::losses::mse(prediction, target) << "\n";
  std::cout << "sample prediction: " << take_range(prediction, 0, 0, 1) << "\n";
  std::cout << "sample target: " << take_range(target, 0, 0, 1) << "\n";

  return 0;
}
