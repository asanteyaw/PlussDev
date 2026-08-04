#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <NodeK/kernels/mps_conv.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key_)) {
      old_ = std::string(old);
    }
    setenv(key_, value, 1);
  }

  ~ScopedEnvVar() {
    if (old_.has_value()) {
      setenv(key_, old_->c_str(), 1);
    } else {
      unsetenv(key_);
    }
  }

private:
  const char* key_;
  std::optional<std::string> old_{};
};

Config f32_cpu() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

Config f32_mps() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
}

double scalar_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

double parameters_l1(const pluss::nn::Schedule& model) {
  double total = 0.0;
  for (const Series* p : model->parameters()) {
    if (!p || !p->defined()) continue;
    total += scalar_f64(p->to(pluss::pCPU, ScalarType::Float64).abs().sum());
  }
  return total;
}

bool parameters_are_mps(const pluss::nn::Schedule& model) {
  for (const Series* p : model->parameters()) {
    if (!p || !p->defined()) continue;
    if (p->platform() != pluss::pMPS || p->scalar_type() != ScalarType::Float32) return false;
  }
  return true;
}

template <class ModuleT>
void seed_parameters(ModuleT& model, float base, float step) {
  for (Series* p : model->parameters()) {
    if (!p || !p->defined()) continue;
    const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(p->platform());
    const Series idx =
        pluss::series::arange(0, p->numel(), 1, icfg).to(p->platform(), ScalarType::Float32).reshape(p->sizes());
    *p = pluss::series::full_like(*p, base, ScalarType::Float32).add(idx.mul(step));
    p->tracks_slope_(true);
  }
}

std::pair<Series, Series> make_fusion_data() {
  constexpr std::int64_t n = 32;
  std::vector<float> xbuf(static_cast<std::size_t>(n * 2));
  std::vector<float> ybuf(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const float x0 = static_cast<float>((i % 11) - 5) / 5.0f;
    const float x1 = static_cast<float>(((i * 5) % 13) - 6) / 6.0f;
    xbuf[static_cast<std::size_t>(i * 2 + 0)] = x0;
    xbuf[static_cast<std::size_t>(i * 2 + 1)] = x1;
    ybuf[static_cast<std::size_t>(i)] = 0.65f * x0 - 0.35f * x1 + 0.1f;
  }
  return {
      pluss::make_series(xbuf, {n, 2}, f32_cpu()),
      pluss::make_series(ybuf, {n, 1}, f32_cpu()),
  };
}

std::pair<Series, Series> make_sequence_data() {
  constexpr std::int64_t batch = 10;
  constexpr std::int64_t steps = 7;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * steps));
  std::vector<float> ybuf(static_cast<std::size_t>(batch * steps));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < steps; ++t) {
      const float x = 0.17f * static_cast<float>(t) + 0.06f * static_cast<float>(b);
      const float v = std::sin(x) + 0.1f * std::cos(2.0f * x);
      xbuf[static_cast<std::size_t>(b * steps + t)] = v;
      ybuf[static_cast<std::size_t>(b * steps + t)] = 0.5f * v + 0.2f * std::sin(1.3f * x);
    }
  }
  return {
      pluss::make_series(xbuf, {batch, steps, 1}, f32_cpu()),
      pluss::make_series(ybuf, {batch, steps, 1}, f32_cpu()),
  };
}

std::pair<Series, Series> make_conv_data() {
  constexpr std::int64_t batch = 12;
  constexpr std::int64_t length = 10;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * length));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < length; ++t) {
      const float x = 0.13f * static_cast<float>(t) + 0.04f * static_cast<float>(b);
      xbuf[static_cast<std::size_t>(b * length + t)] = std::sin(x) + 0.25f * std::cos(2.0f * x);
    }
  }
  const Series x = pluss::make_series(xbuf, {batch, 1, length}, f32_cpu());
  const Series kernel = pluss::make_series(std::vector<float>{0.2f, -0.1f, 0.35f}, {1, 1, 3}, f32_cpu());
  const Series bias = pluss::make_series(std::vector<float>{0.03f}, {1}, f32_cpu());
  return {x, pluss::conv1d(x, kernel, bias, 1, 1, 1, 1)};
}

std::pair<Series, Series> make_conv2d_data() {
  constexpr std::int64_t batch = 8;
  constexpr std::int64_t height = 5;
  constexpr std::int64_t width = 6;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * height * width));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t h = 0; h < height; ++h) {
      for (std::int64_t w = 0; w < width; ++w) {
        const float x = 0.11f * static_cast<float>(h) + 0.07f * static_cast<float>(w) +
                        0.03f * static_cast<float>(b);
        xbuf[static_cast<std::size_t>(((b * height) + h) * width + w)] =
            std::sin(x) + 0.2f * std::cos(2.3f * x);
      }
    }
  }
  const Series x = pluss::make_series(xbuf, {batch, 1, height, width}, f32_cpu());
  const Series kernel = pluss::make_series(std::vector<float>{
                                               0.05f, -0.10f, 0.15f,
                                               0.20f, 0.00f, -0.05f,
                                               -0.10f, 0.25f, 0.10f},
                                           {1, 1, 3, 3},
                                           f32_cpu());
  const Series bias = pluss::make_series(std::vector<float>{0.04f}, {1}, f32_cpu());
  return {x, pluss::conv(x, kernel, bias, std::array<std::int64_t, 2>{1, 1}, std::array<std::int64_t, 2>{1, 1})};
}

std::pair<Series, Series> make_grouped_conv1d_data() {
  constexpr std::int64_t batch = 10;
  constexpr std::int64_t channels = 4;
  constexpr std::int64_t length = 9;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * channels * length));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t c = 0; c < channels; ++c) {
      for (std::int64_t t = 0; t < length; ++t) {
        const float x = 0.05f * static_cast<float>(b) + 0.09f * static_cast<float>(c) + 0.13f * static_cast<float>(t);
        xbuf[static_cast<std::size_t>(((b * channels) + c) * length + t)] = std::sin(x) + 0.15f * std::cos(1.9f * x);
      }
    }
  }
  const Series x = pluss::make_series(xbuf, {batch, channels, length}, f32_cpu());
  std::vector<float> wbuf(static_cast<std::size_t>(4 * 2 * 3));
  for (std::size_t i = 0; i < wbuf.size(); ++i) {
    wbuf[i] = -0.12f + 0.02f * static_cast<float>(i % 13);
  }
  const Series kernel = pluss::make_series(wbuf, {4, 2, 3}, f32_cpu());
  const Series bias = pluss::make_series(std::vector<float>{0.01f, -0.02f, 0.03f, -0.01f}, {4}, f32_cpu());
  return {x, pluss::conv1d(x, kernel, bias, 1, 1, 1, 2)};
}

std::pair<Series, Series> make_depthwise_conv2d_data() {
  constexpr std::int64_t batch = 6;
  constexpr std::int64_t channels = 3;
  constexpr std::int64_t height = 5;
  constexpr std::int64_t width = 5;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * channels * height * width));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t c = 0; c < channels; ++c) {
      for (std::int64_t h = 0; h < height; ++h) {
        for (std::int64_t w = 0; w < width; ++w) {
          const float x = 0.04f * static_cast<float>(b) + 0.08f * static_cast<float>(c) +
                          0.11f * static_cast<float>(h) + 0.07f * static_cast<float>(w);
          xbuf[static_cast<std::size_t>((((b * channels) + c) * height + h) * width + w)] =
              std::sin(x) + 0.1f * std::cos(2.1f * x);
        }
      }
    }
  }
  const Series x = pluss::make_series(xbuf, {batch, channels, height, width}, f32_cpu());
  std::vector<float> wbuf(static_cast<std::size_t>(channels * 1 * 3 * 3));
  for (std::size_t i = 0; i < wbuf.size(); ++i) {
    wbuf[i] = -0.08f + 0.015f * static_cast<float>(i % 11);
  }
  const Series kernel = pluss::make_series(wbuf, {channels, 1, 3, 3}, f32_cpu());
  const Series bias = pluss::make_series(std::vector<float>{0.02f, -0.01f, 0.015f}, {channels}, f32_cpu());
  return {
      x,
      pluss::conv(x, kernel, bias, std::array<std::int64_t, 2>{1, 1}, std::array<std::int64_t, 2>{1, 1}, std::array<std::int64_t, 2>{1, 1}, channels)};
}

std::pair<Series, Series> make_causal_conv1d_data() {
  constexpr std::int64_t batch = 9;
  constexpr std::int64_t length = 12;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * length));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < length; ++t) {
      const float x = 0.06f * static_cast<float>(b) + 0.17f * static_cast<float>(t);
      xbuf[static_cast<std::size_t>(b * length + t)] = std::sin(x) + 0.2f * std::cos(1.4f * x);
    }
  }
  const Series x = pluss::make_series(xbuf, {batch, 1, length}, f32_cpu());
  const Series kernel = pluss::make_series(std::vector<float>{0.15f, -0.05f, 0.25f}, {1, 1, 3}, f32_cpu());
  const Series bias = pluss::make_series(std::vector<float>{0.02f}, {1}, f32_cpu());
  return {x, pluss::conv1d(x, kernel, bias, pluss::causal_padding1d(3, 2), 1, 2, 1)};
}

std::pair<Series, Series> make_conv3d_data() {
  constexpr std::int64_t batch = 5;
  constexpr std::int64_t channels = 2;
  constexpr std::int64_t depth = 4;
  constexpr std::int64_t height = 4;
  constexpr std::int64_t width = 5;
  std::vector<float> xbuf(static_cast<std::size_t>(batch * channels * depth * height * width));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t c = 0; c < channels; ++c) {
      for (std::int64_t d = 0; d < depth; ++d) {
        for (std::int64_t h = 0; h < height; ++h) {
          for (std::int64_t w = 0; w < width; ++w) {
            const float x = 0.03f * static_cast<float>(b) + 0.07f * static_cast<float>(c) +
                            0.09f * static_cast<float>(d) + 0.05f * static_cast<float>(h) +
                            0.04f * static_cast<float>(w);
            xbuf[static_cast<std::size_t>(((((b * channels) + c) * depth + d) * height + h) * width + w)] =
                std::sin(x) + 0.12f * std::cos(1.8f * x);
          }
        }
      }
    }
  }
  const Series x = pluss::make_series(xbuf, {batch, channels, depth, height, width}, f32_cpu());
  std::vector<float> wbuf(static_cast<std::size_t>(2 * 2 * 3 * 3 * 3));
  for (std::size_t i = 0; i < wbuf.size(); ++i) {
    wbuf[i] = -0.04f + 0.01f * static_cast<float>(i % 9);
  }
  const Series kernel = pluss::make_series(wbuf, {2, 2, 3, 3, 3}, f32_cpu());
  const Series bias = pluss::make_series(std::vector<float>{0.01f, -0.015f}, {2}, f32_cpu());
  return {
      x,
      pluss::conv3d(
          x,
          kernel,
          bias,
          std::array<std::int64_t, 3>{1, 1, 1},
          std::array<std::int64_t, 3>{1, 1, 1},
          std::array<std::int64_t, 3>{1, 1, 1},
          1)};
}

std::pair<Series, Series> make_transformer_data() {
  constexpr std::int64_t batch = 5;
  constexpr std::int64_t seq = 4;
  constexpr std::int64_t d_model = 8;
  std::vector<float> values(static_cast<std::size_t>(batch * seq * d_model));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < seq; ++t) {
      for (std::int64_t d = 0; d < d_model; ++d) {
        const float x = 0.09f * static_cast<float>(t) + 0.05f * static_cast<float>(d) + 0.03f * static_cast<float>(b);
        values[static_cast<std::size_t>(((b * seq) + t) * d_model + d)] =
            std::sin(x) + 0.1f * std::cos(1.7f * x);
      }
    }
  }
  const Series src = pluss::make_series(values, {batch, seq, d_model}, f32_cpu());
  return {src, src};
}

struct RNNProjectorImpl final : pluss::nn::Module {
  pluss::nn::RNN rnn;
  pluss::nn::Fusion head;

  explicit RNNProjectorImpl(const Config& cfg)
      : Module(true),
        rnn(pluss::nn::RNNOptions{
            .input_size = 1,
            .hidden_size = 6,
            .num_layers = 1,
            .nonlinearity = "tanh",
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .cfg = cfg,
            .seed = 101ULL,
            .training = true,
        }),
        head(6, 1, true, cfg, 103ULL) {
    module_record("rnn", rnn);
    module_record("head", head);
  }

  Series forward(const Series& input) const {
    const auto [seq, state] = rnn->forward(input);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(RNNProjector);

struct GRUProjectorImpl final : pluss::nn::Module {
  pluss::nn::GRU gru;
  pluss::nn::Fusion head;

  explicit GRUProjectorImpl(const Config& cfg)
      : Module(true),
        gru(pluss::nn::GRUOptions{
            .input_size = 1,
            .hidden_size = 6,
            .num_layers = 1,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .cfg = cfg,
            .seed = 107ULL,
            .training = true,
        }),
        head(6, 1, true, cfg, 109ULL) {
    module_record("gru", gru);
    module_record("head", head);
  }

  Series forward(const Series& input) const {
    const auto [seq, state] = gru->forward(input);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(GRUProjector);

struct LSTMProjectorImpl final : pluss::nn::Module {
  pluss::nn::LSTM lstm;
  pluss::nn::Fusion head;

  explicit LSTMProjectorImpl(const Config& cfg)
      : Module(true),
        lstm(pluss::nn::LSTMOptions{
            .input_size = 1,
            .hidden_size = 6,
            .num_layers = 1,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .proj_size = 0,
            .cfg = cfg,
            .seed = 113ULL,
            .training = true,
        }),
        head(6, 1, true, cfg, 127ULL) {
    module_record("lstm", lstm);
    module_record("head", head);
  }

  Series forward(const Series& input) const {
    const auto [seq, state] = lstm->forward(input);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(LSTMProjector);

struct StackedBidirectionalRNNProjectorImpl final : pluss::nn::Module {
  pluss::nn::RNN rnn;
  pluss::nn::Fusion head;

  explicit StackedBidirectionalRNNProjectorImpl(const Config& cfg)
      : Module(true),
        rnn(pluss::nn::RNNOptions{
            .input_size = 1,
            .hidden_size = 4,
            .num_layers = 2,
            .nonlinearity = "tanh",
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = true,
            .split_bias = true,
            .cfg = cfg,
            .seed = 137ULL,
            .training = true,
        }),
        head(8, 1, true, cfg, 139ULL) {
    module_record("rnn", rnn);
    module_record("head", head);
  }

  Series forward(const Series& input) const {
    const auto [seq, state] = rnn->forward(input);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(StackedBidirectionalRNNProjector);

struct StackedBidirectionalGRUProjectorImpl final : pluss::nn::Module {
  pluss::nn::GRU gru;
  pluss::nn::Fusion head;

  explicit StackedBidirectionalGRUProjectorImpl(const Config& cfg)
      : Module(true),
        gru(pluss::nn::GRUOptions{
            .input_size = 1,
            .hidden_size = 4,
            .num_layers = 2,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = true,
            .split_bias = true,
            .cfg = cfg,
            .seed = 149ULL,
            .training = true,
        }),
        head(8, 1, true, cfg, 151ULL) {
    module_record("gru", gru);
    module_record("head", head);
  }

  Series forward(const Series& input) const {
    const auto [seq, state] = gru->forward(input);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(StackedBidirectionalGRUProjector);

struct StackedBidirectionalLSTMProjectorImpl final : pluss::nn::Module {
  pluss::nn::LSTM lstm;
  pluss::nn::Fusion head;

  explicit StackedBidirectionalLSTMProjectorImpl(const Config& cfg)
      : Module(true),
        lstm(pluss::nn::LSTMOptions{
            .input_size = 1,
            .hidden_size = 4,
            .num_layers = 2,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = true,
            .split_bias = true,
            .proj_size = 0,
            .cfg = cfg,
            .seed = 157ULL,
            .training = true,
        }),
        head(8, 1, true, cfg, 163ULL) {
    module_record("lstm", lstm);
    module_record("head", head);
  }

  Series forward(const Series& input) const {
    const auto [seq, state] = lstm->forward(input);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(StackedBidirectionalLSTMProjector);

struct TransformerAutoencoderImpl final : pluss::nn::Module {
  pluss::nn::Transformer transformer;

  explicit TransformerAutoencoderImpl(const Config& cfg)
      : Module(true),
        transformer(pluss::nn::TransformerOptions{
            .d_model = 8,
            .nhead = 2,
            .num_encoder_layers = 1,
            .num_decoder_layers = 1,
            .dim_feedforward = 16,
            .dropout = 0.0,
            .activation = pluss::nn::TransformerActivation::ReLU,
            .layer_norm_eps = 1e-5,
            .batch_first = true,
            .norm_first = false,
            .bias = true,
            .cfg = cfg,
            .seed = 131ULL,
            .training = true,
        }) {
    module_record("transformer", transformer);
  }

  Series forward(const Series& input) const {
    return transformer->forward(input, input);
  }
};

PLUSS_MODULE(TransformerAutoencoder);

struct MaskedTransformerAutoencoderImpl final : pluss::nn::Module {
  pluss::nn::Transformer transformer;

  explicit MaskedTransformerAutoencoderImpl(const Config& cfg)
      : Module(true),
        transformer(pluss::nn::TransformerOptions{
            .d_model = 8,
            .nhead = 2,
            .num_encoder_layers = 2,
            .num_decoder_layers = 2,
            .dim_feedforward = 24,
            .dropout = 0.0,
            .activation = pluss::nn::TransformerActivation::GELU,
            .layer_norm_eps = 1e-5,
            .batch_first = true,
            .norm_first = true,
            .bias = true,
            .cfg = cfg,
            .seed = 167ULL,
            .training = true,
        }) {
    module_record("transformer", transformer);
  }

  Series forward(const Series& input) const {
    const std::int64_t batch = input.sizes()[0];
    const std::int64_t seq = input.sizes()[1];
    const Series seq_mask = pluss::square_subsequent_mask(seq, input);
    const Series pad_mask = pluss::series::zeros({batch, seq}, Config{}.with_dtype(ScalarType::Bool).with_platform(input.platform()));
    return transformer->forward(
        input,
        input,
        seq_mask,
        seq_mask,
        seq_mask,
        pad_mask,
        pad_mask,
        pad_mask,
        true,
        true,
        false);
  }
};

PLUSS_MODULE(MaskedTransformerAutoencoder);

bool train_schedule_case(const char* label,
                         pluss::nn::Schedule& model,
                         const Series& x_cpu,
                         const Series& y_cpu,
                         int epochs,
                         double learning_rate,
                         std::ostream* err) {
  const Series x = x_cpu.to(pluss::pMPS, ScalarType::Float32);
  const Series y = y_cpu.to(pluss::pMPS, ScalarType::Float32);
  model->train(true);
  model->to(pluss::pMPS, ScalarType::Float32);
  seed_parameters(model, 0.02f, 1e-4f);

  if (model->parameters().empty()) {
    if (err) *err << label << " has no trainable parameters\n";
    return false;
  }
  if (!parameters_are_mps(model)) {
    if (err) *err << label << " parameters are not on MPS before training\n";
    return false;
  }

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = learning_rate;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(*model, opt);
  pluss::nn::optim::StepDiagnosticsOptions diag{};
  diag.fail_if_no_sensitivity = true;
  optimizer.set_step_diagnostics(diag);

  const double before_l1 = parameters_l1(model);
  double first_loss = 0.0;
  double last_loss = 0.0;
  for (int epoch = 0; epoch < epochs; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      return pluss::nn::losses::MSE(model->forward(x), y);
    });
    const double value = scalar_f64(loss);
    if (!std::isfinite(value)) {
      if (err) *err << label << " produced non-finite loss at epoch " << (epoch + 1) << "\n";
      return false;
    }
    if (epoch == 0) first_loss = value;
    last_loss = value;
  }

  const double after_l1 = parameters_l1(model);
  if (!parameters_are_mps(model)) {
    if (err) *err << label << " parameters left MPS during training\n";
    return false;
  }
  if (!(last_loss < first_loss)) {
    if (err) *err << label << " did not reduce loss (first=" << first_loss << ", last=" << last_loss << ")\n";
    return false;
  }
  if (std::abs(after_l1 - before_l1) <= 1e-7) {
    if (err) *err << label << " parameters did not change\n";
    return false;
  }
  return true;
}

bool run_mps_schedule_family_trainability(std::ostream* err = nullptr) {
  if (!pluss::mps::is_ready()) {
    return true;
  }

  const ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar no_conv_bridge("PLUSS_MPS_CONV_CPU_BRIDGE", "0");
  const ScopedEnvVar strict_conv3d("PLUSS_MPS_CONV3D_ROUTE_STRICT", "1");
  const ScopedEnvVar no_matmul_bridge("PLUSS_MPS_MATMUL_CPU_BRIDGE", "0");
  const ScopedEnvVar no_index_backward_bridge("PLUSS_MPS_INDEX_SELECT_CPU_BACKWARD", "0");
  const ScopedEnvVar no_linalg_bridge("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
  const ScopedEnvVar native_sort("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  const Config cfg = f32_mps();

  {
    const auto [x, y] = make_fusion_data();
    pluss::nn::Schedule model;
    model->then("fusion0", pluss::nn::Fusion(2, 8, true, cfg, 11ULL))
        .then("activation", pluss::nn::Tanh())
        .then("fusion1", pluss::nn::Fusion(8, 1, true, cfg, 13ULL));
    if (!train_schedule_case("Schedule<Fusion>", model, x, y, 12, 2e-2, err)) return false;
  }

  {
    const auto [x, y] = make_sequence_data();
    pluss::nn::Schedule model;
    model->then("rnn", RNNProjector(cfg));
    if (!train_schedule_case("Schedule<RNN>", model, x, y, 12, 4e-3, err)) return false;
  }

  {
    const auto [x, y] = make_sequence_data();
    pluss::nn::Schedule model;
    model->then("stacked_bidirectional_rnn", StackedBidirectionalRNNProjector(cfg));
    if (!train_schedule_case("Schedule<StackedBidirectionalRNN>", model, x, y, 10, 3e-3, err)) return false;
  }

  {
    const auto [x, y] = make_sequence_data();
    pluss::nn::Schedule model;
    model->then("gru", GRUProjector(cfg));
    if (!train_schedule_case("Schedule<GRU>", model, x, y, 12, 4e-3, err)) return false;
  }

  {
    const auto [x, y] = make_sequence_data();
    pluss::nn::Schedule model;
    model->then("stacked_bidirectional_gru", StackedBidirectionalGRUProjector(cfg));
    if (!train_schedule_case("Schedule<StackedBidirectionalGRU>", model, x, y, 10, 3e-3, err)) return false;
  }

  {
    const auto [x, y] = make_sequence_data();
    pluss::nn::Schedule model;
    model->then("lstm", LSTMProjector(cfg));
    if (!train_schedule_case("Schedule<LSTM>", model, x, y, 12, 2e-3, err)) return false;
  }

  {
    const auto [x, y] = make_sequence_data();
    pluss::nn::Schedule model;
    model->then("stacked_bidirectional_lstm", StackedBidirectionalLSTMProjector(cfg));
    if (!train_schedule_case("Schedule<StackedBidirectionalLSTM>", model, x, y, 10, 2e-3, err)) return false;
  }

  {
    const auto [x, y] = make_conv_data();
    pluss::nn::Schedule model;
    model->then("conv", pluss::nn::Conv1d(pluss::nn::Conv1dOptions{
                            .in_channels = 1,
                            .out_channels = 1,
                            .kernel_size = 3,
                            .stride = 1,
                            .padding = 1,
                            .dilation = 1,
                            .groups = 1,
                            .bias = true,
                            .cfg = cfg,
                            .seed = 17ULL,
                        }));
    if (!train_schedule_case("Schedule<Conv1d>", model, x, y, 12, 1e-2, err)) return false;
  }

  {
    const auto [x, y] = make_grouped_conv1d_data();
    pluss::nn::Schedule model;
    model->then("grouped_conv1d", pluss::nn::Conv1d(pluss::nn::Conv1dOptions{
                                    .in_channels = 4,
                                    .out_channels = 4,
                                    .kernel_size = 3,
                                    .stride = 1,
                                    .padding = 1,
                                    .dilation = 1,
                                    .groups = 2,
                                    .bias = true,
                                    .cfg = cfg,
                                    .seed = 23ULL,
                                }));
    if (!train_schedule_case("Schedule<GroupedConv1d>", model, x, y, 10, 8e-3, err)) return false;
  }

  {
    const auto [x, y] = make_causal_conv1d_data();
    pluss::nn::Schedule model;
    model->then("causal_conv1d", pluss::nn::Conv1d(pluss::nn::Conv1dOptions{
                                   .in_channels = 1,
                                   .out_channels = 1,
                                   .kernel_size = 3,
                                   .stride = 1,
                                   .padding = 0,
                                   .padding_lr = pluss::causal_padding1d(3, 2),
                                   .dilation = 2,
                                   .groups = 1,
                                   .bias = true,
                                   .cfg = cfg,
                                   .seed = 29ULL,
                               }));
    if (!train_schedule_case("Schedule<CausalDilatedConv1d>", model, x, y, 10, 8e-3, err)) return false;
  }

  {
    const auto [x, y] = make_conv2d_data();
    pluss::nn::Schedule model;
    model->then("conv2d", pluss::nn::Conv(pluss::nn::ConvOptions{
                              .in_channels = 1,
                              .out_channels = 1,
                              .kernel_size = {3, 3},
                              .stride = {1, 1},
                              .padding = {1, 1},
                              .dilation = {1, 1},
                              .groups = 1,
                              .bias = true,
                              .cfg = cfg,
                              .seed = 19ULL,
                          }));
    if (!train_schedule_case("Schedule<Conv2d>", model, x, y, 12, 8e-3, err)) return false;
  }

  {
    const auto [x, y] = make_depthwise_conv2d_data();
    pluss::nn::Schedule model;
    model->then("depthwise_conv2d", pluss::nn::Conv(pluss::nn::ConvOptions{
                                        .in_channels = 3,
                                        .out_channels = 3,
                                        .kernel_size = {3, 3},
                                        .stride = {1, 1},
                                        .padding = {1, 1},
                                        .dilation = {1, 1},
                                        .groups = 3,
                                        .bias = true,
                                        .cfg = cfg,
                                        .seed = 31ULL,
                                    }));
    if (!train_schedule_case("Schedule<DepthwiseConv2d>", model, x, y, 10, 8e-3, err)) return false;
  }

  {
    const auto [x, y] = make_conv3d_data();
    pluss::nn::Schedule model;
    const std::uint64_t native_before = pluss::ai::kernels::mps::conv3d_native_call_count();
    model->then("conv3d", pluss::nn::Conv3d(pluss::nn::Conv3dOptions{
                                      .in_channels = 2,
                                      .out_channels = 2,
                                      .kernel_size = {3, 3, 3},
                                      .stride = {1, 1, 1},
                                      .padding = {1, 1, 1},
                                      .dilation = {1, 1, 1},
                                      .groups = 1,
                                      .bias = true,
                                      .cfg = cfg,
                                      .seed = 37ULL,
                                  }));
    if (!train_schedule_case("Schedule<Conv3dNative>", model, x, y, 10, 6e-3, err)) return false;
    if (pluss::ai::kernels::mps::conv3d_native_call_count() <= native_before) {
      if (err) *err << "Schedule<Conv3dNative> did not enter the native MPSGraph Conv3d route\n";
      return false;
    }
  }

  {
    const auto [x, y] = make_transformer_data();
    pluss::nn::Schedule model;
    model->then("transformer", TransformerAutoencoder(cfg));
    if (!train_schedule_case("Schedule<Transformer>", model, x, y, 8, 3e-3, err)) return false;
  }

  {
    const auto [x, y] = make_transformer_data();
    pluss::nn::Schedule model;
    model->then("masked_transformer", MaskedTransformerAutoencoder(cfg));
    if (!train_schedule_case("Schedule<MaskedTransformer>", model, x, y, 6, 2e-3, err)) return false;
  }

  {
    const auto [x, y] = make_fusion_data();
    pluss::nn::Schedule model;
    model->then("stress_fusion0", pluss::nn::Fusion(2, 10, true, cfg, 191ULL))
        .then("stress_activation", pluss::nn::Tanh())
        .then("stress_fusion1", pluss::nn::Fusion(10, 1, true, cfg, 193ULL));
    if (!train_schedule_case("Schedule<FusionStress64>", model, x, y, 64, 8e-3, err)) return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn.Schedule trains every core DL family on MPS", "[mps][schedule][training]") {
  REQUIRE(run_mps_schedule_family_trainability(&std::cerr));
}
#else
int main() {
  if (!run_mps_schedule_family_trainability(&std::cerr)) return 1;
  std::cout << "nn_mps_schedule_family_trainability_test passed\n";
  return 0;
}
#endif
