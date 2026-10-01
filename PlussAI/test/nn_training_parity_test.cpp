#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::Platform;
using pluss::ScalarType;
using pluss::Series;

bool env_flag_is_true_(const char* name) {
  const char* value = std::getenv(name);
  if (!value) return false;
  const std::string s(value);
  return s == "1" || s == "true" || s == "TRUE" || s == "on" || s == "ON";
}

int env_int_or_(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (!value) return fallback;
  try {
    return std::stoi(value);
  } catch (...) {
    return fallback;
  }
}

double env_double_or_(const char* name, double fallback) {
  const char* value = std::getenv(name);
  if (!value) return fallback;
  try {
    return std::stod(value);
  } catch (...) {
    return fallback;
  }
}

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key_)) {
      old_ = std::string(old);
    }
#if defined(_WIN32)
    _putenv_s(key_, value);
#else
    setenv(key_, value, 1);
#endif
  }

  ~ScopedEnvVar() {
    if (old_.has_value()) {
#if defined(_WIN32)
      _putenv_s(key_, old_->c_str());
#else
      setenv(key_, old_->c_str(), 1);
#endif
    } else {
#if defined(_WIN32)
      _putenv_s(key_, "");
#else
      unsetenv(key_);
#endif
    }
  }

private:
  const char* key_;
  std::optional<std::string> old_{};
};

const char* platform_name_(Platform platform) noexcept {
  switch (platform) {
    case pluss::pCPU: return "cpu";
    case pluss::pMPS: return "mps";
    case pluss::pCUDA: return "cuda";
    default: return "unknown";
  }
}

enum class ParityBackendMode {
  All,
  CPU,
  MPS,
  CUDA,
};

ParityBackendMode parity_backend_mode_() {
  const char* raw = std::getenv("PLUSS_PARITY_BACKEND");
  if (!raw) return ParityBackendMode::All;
  const std::string mode(raw);
  if (mode == "cpu" || mode == "CPU") return ParityBackendMode::CPU;
  if (mode == "mps" || mode == "MPS") return ParityBackendMode::MPS;
  if (mode == "cuda" || mode == "CUDA") return ParityBackendMode::CUDA;
  return ParityBackendMode::All;
}

struct TrainingTrace {
  std::vector<double> epoch_loss{};
  std::vector<double> flat_parameters{};
};

enum class LinearOptimizerKind {
  Adam,
  AdamW,
  SGD,
  RMSprop,
  Adagrad,
  NAdam,
};

struct TraceTolerance {
  double loss_atol;
  double loss_rtol;
  double param_atol;
  double param_rtol;
};

bool approx(double a, double b, double atol, double rtol) {
  return std::abs(a - b) <= (atol + rtol * std::abs(b));
}

const char* linear_optimizer_name(LinearOptimizerKind kind) {
  switch (kind) {
    case LinearOptimizerKind::Adam: return "Adam";
    case LinearOptimizerKind::AdamW: return "AdamW";
    case LinearOptimizerKind::SGD: return "SGD";
    case LinearOptimizerKind::RMSprop: return "RMSprop";
    case LinearOptimizerKind::Adagrad: return "Adagrad";
    case LinearOptimizerKind::NAdam: return "NAdam";
    default: return "Unknown";
  }
}

const char* torch_env_for_linear_optimizer(LinearOptimizerKind kind) {
  switch (kind) {
    case LinearOptimizerKind::Adam: return nullptr;
    case LinearOptimizerKind::AdamW: return "PLUSS_TORCH_LINEAR_ADAMW_REF_CSV";
    case LinearOptimizerKind::SGD: return "PLUSS_TORCH_LINEAR_SGD_REF_CSV";
    case LinearOptimizerKind::RMSprop: return "PLUSS_TORCH_LINEAR_RMSPROP_REF_CSV";
    case LinearOptimizerKind::Adagrad: return "PLUSS_TORCH_LINEAR_ADAGRAD_REF_CSV";
    case LinearOptimizerKind::NAdam: return "PLUSS_TORCH_LINEAR_NADAM_REF_CSV";
    default: return nullptr;
  }
}

const char* torch_env_for_linear_optimizer_mps(LinearOptimizerKind kind) {
  switch (kind) {
    case LinearOptimizerKind::Adam: return "PLUSS_TORCH_MPS_LINEAR_REF_CSV";
    case LinearOptimizerKind::AdamW: return "PLUSS_TORCH_MPS_LINEAR_ADAMW_REF_CSV";
    case LinearOptimizerKind::SGD: return "PLUSS_TORCH_MPS_LINEAR_SGD_REF_CSV";
    case LinearOptimizerKind::RMSprop: return "PLUSS_TORCH_MPS_LINEAR_RMSPROP_REF_CSV";
    case LinearOptimizerKind::Adagrad: return "PLUSS_TORCH_MPS_LINEAR_ADAGRAD_REF_CSV";
    case LinearOptimizerKind::NAdam: return "PLUSS_TORCH_MPS_LINEAR_NADAM_REF_CSV";
    default: return nullptr;
  }
}

const char* torch_env_for_linear_optimizer_cuda(LinearOptimizerKind kind) {
  switch (kind) {
    case LinearOptimizerKind::Adam: return "PLUSS_TORCH_CUDA_LINEAR_REF_CSV";
    case LinearOptimizerKind::AdamW: return "PLUSS_TORCH_CUDA_LINEAR_ADAMW_REF_CSV";
    case LinearOptimizerKind::SGD: return "PLUSS_TORCH_CUDA_LINEAR_SGD_REF_CSV";
    case LinearOptimizerKind::RMSprop: return "PLUSS_TORCH_CUDA_LINEAR_RMSPROP_REF_CSV";
    case LinearOptimizerKind::Adagrad: return "PLUSS_TORCH_CUDA_LINEAR_ADAGRAD_REF_CSV";
    case LinearOptimizerKind::NAdam: return "PLUSS_TORCH_CUDA_LINEAR_NADAM_REF_CSV";
    default: return nullptr;
  }
}

std::pair<double, double> torch_linear_loss_tolerance(LinearOptimizerKind kind) {
  switch (kind) {
    case LinearOptimizerKind::Adam:
      return {2e-3, 0.25};
    case LinearOptimizerKind::AdamW:
      return {3e-3, 0.30};
    case LinearOptimizerKind::SGD:
      return {6e-3, 0.35};
    case LinearOptimizerKind::RMSprop:
      return {5e-3, 0.35};
    case LinearOptimizerKind::Adagrad:
      return {5e-3, 0.35};
    case LinearOptimizerKind::NAdam:
      return {3e-3, 0.30};
    default:
      return {5e-3, 0.35};
  }
}

TraceTolerance backend_linear_tolerance(LinearOptimizerKind kind) {
  switch (kind) {
    case LinearOptimizerKind::Adam:
      return {2e-3, 0.20, 2e-2, 0.10};
    case LinearOptimizerKind::NAdam:
      return {4e-3, 0.30, 4e-2, 0.18};
    default:
      return {7e-3, 0.40, 6e-2, 0.25};
  }
}

bool trace_is_finite(const TrainingTrace& trace) {
  for (double v : trace.epoch_loss) {
    if (!std::isfinite(v)) return false;
  }
  for (double v : trace.flat_parameters) {
    if (!std::isfinite(v)) return false;
  }
  return true;
}

Config f32_cpu() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

std::pair<Series, Series> make_linear_data() {
  const Config cfg = f32_cpu();
  constexpr std::int64_t n = 256;
  std::vector<float> xbuf(static_cast<std::size_t>(n * 2));
  std::vector<float> ybuf(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const float x0 = static_cast<float>((i % 17) - 8) / 8.0f;
    const float x1 = static_cast<float>(((i * 7) % 19) - 9) / 9.0f;
    xbuf[static_cast<std::size_t>(i * 2 + 0)] = x0;
    xbuf[static_cast<std::size_t>(i * 2 + 1)] = x1;
    ybuf[static_cast<std::size_t>(i)] = (1.75f * x0) - (2.25f * x1) + 0.35f;
  }
  const Series x = pluss::make_series(xbuf, {n, 2}, cfg);
  const Series y = pluss::make_series(ybuf, {n, 1}, cfg);
  return {x, y};
}

struct TinyLSTMParityRegressorImpl final : pluss::nn::Module {
  pluss::nn::LSTM lstm;
  pluss::nn::Fusion head;
  mutable Series last_seq = pluss::series::undefined();

  explicit TinyLSTMParityRegressorImpl(const Config& cfg)
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
    const auto [seq, state] = lstm->forward(x);
    (void)state;
    last_seq = seq;
    return head->forward(seq);
  }
};

PLUSS_MODULE(TinyLSTMParityRegressor);

struct TinyRNNParityRegressorImpl final : pluss::nn::Module {
  pluss::nn::RNN rnn;
  pluss::nn::Fusion head;

  explicit TinyRNNParityRegressorImpl(const Config& cfg)
      : Module(true),
        rnn(pluss::nn::RNNOptions{
            .input_size = 1,
            .hidden_size = 10,
            .num_layers = 1,
            .nonlinearity = "tanh",
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .cfg = cfg,
            .seed = 29ULL,
            .training = true,
        }),
        head(10, 1, true, cfg, 31ULL) {
    module_record("rnn", rnn);
    module_record("head", head);
  }

  Series forward(const Series& x) const {
    const auto [seq, state] = rnn->forward(x);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(TinyRNNParityRegressor);

struct TinyGRUParityRegressorImpl final : pluss::nn::Module {
  pluss::nn::GRU gru;
  pluss::nn::Fusion head;

  explicit TinyGRUParityRegressorImpl(const Config& cfg)
      : Module(true),
        gru(pluss::nn::GRUOptions{
            .input_size = 1,
            .hidden_size = 10,
            .num_layers = 1,
            .bias = true,
            .batch_first = true,
            .dropout = 0.0,
            .bidirectional = false,
            .split_bias = true,
            .cfg = cfg,
            .seed = 33ULL,
            .training = true,
        }),
        head(10, 1, true, cfg, 35ULL) {
    module_record("gru", gru);
    module_record("head", head);
  }

  Series forward(const Series& x) const {
    const auto [seq, state] = gru->forward(x);
    (void)state;
    return head->forward(seq);
  }
};

PLUSS_MODULE(TinyGRUParityRegressor);

Series take_range(const Series& x, std::int64_t dim, std::int64_t start, std::int64_t length) {
  std::vector<std::int64_t> idx(static_cast<std::size_t>(length));
  for (std::int64_t i = 0; i < length; ++i) idx[static_cast<std::size_t>(i)] = start + i;
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  return x.index_select(dim, pluss::make_series(idx, {length}, icfg));
}

std::pair<Series, Series> make_lstm_data() {
  const Config cfg = f32_cpu();
  constexpr std::int64_t batch = 24;
  constexpr std::int64_t steps = 20;
  std::vector<float> values(static_cast<std::size_t>(batch * steps));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < steps; ++t) {
      const float x = 0.15f * static_cast<float>(t) + 0.07f * static_cast<float>(b);
      values[static_cast<std::size_t>(b * steps + t)] = std::sin(x) + 0.1f * std::cos(2.0f * x);
    }
  }
  const Series seq = pluss::make_series(values, {batch, steps, 1}, cfg);
  return {take_range(seq, 1, 0, 19), take_range(seq, 1, 1, 19)};
}

std::pair<Series, Series> make_rnn_data() {
  const Config cfg = f32_cpu();
  constexpr std::int64_t batch = 20;
  constexpr std::int64_t steps = 18;
  std::vector<float> values(static_cast<std::size_t>(batch * steps));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < steps; ++t) {
      const float x = 0.11f * static_cast<float>(t) + 0.09f * static_cast<float>(b);
      values[static_cast<std::size_t>(b * steps + t)] = std::sin(x) + 0.05f * std::cos(3.0f * x);
    }
  }
  const Series seq = pluss::make_series(values, {batch, steps, 1}, cfg);
  return {take_range(seq, 1, 0, 17), take_range(seq, 1, 1, 17)};
}

std::pair<Series, Series> make_conv_data() {
  const Config cfg = f32_cpu();
  constexpr std::int64_t batch = 24;
  constexpr std::int64_t length = 15;
  std::vector<float> values(static_cast<std::size_t>(batch * length));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < length; ++t) {
      const float x = 0.17f * static_cast<float>(t) + 0.05f * static_cast<float>(b);
      values[static_cast<std::size_t>(b * length + t)] = std::sin(x) + 0.2f * std::cos(2.0f * x);
    }
  }
  const Series input = pluss::make_series(values, {batch, 1, length}, cfg);
  const Series kernel = pluss::make_series(std::vector<float>{0.25f, -0.1f, 0.35f}, {1, 1, 3}, cfg);
  const Series bias = pluss::make_series(std::vector<float>{0.05f}, {1}, cfg);
  const Series target = pluss::nn::conv1d(input, kernel, bias, 1, 1, 1, 1);
  return {input, target};
}

std::pair<Series, Series> make_transformer_autoencode_data() {
  const Config cfg = f32_cpu();
  constexpr std::int64_t batch = 8;
  constexpr std::int64_t seq = 6;
  constexpr std::int64_t d_model = 8;
  std::vector<float> values(static_cast<std::size_t>(batch * seq * d_model));
  for (std::int64_t b = 0; b < batch; ++b) {
    for (std::int64_t t = 0; t < seq; ++t) {
      for (std::int64_t d = 0; d < d_model; ++d) {
        const float x = 0.09f * static_cast<float>(t) + 0.07f * static_cast<float>(d) + 0.03f * static_cast<float>(b);
        const float v = std::sin(x) + 0.15f * std::cos(1.7f * x);
        values[static_cast<std::size_t>(((b * seq) + t) * d_model + d)] = v;
      }
    }
  }
  const Series src = pluss::make_series(values, {batch, seq, d_model}, cfg);
  return {src, src};
}

std::pair<Series, Series> make_custom_objective_data() {
  const Config cfg = f32_cpu();
  constexpr std::int64_t n = 16;
  std::vector<float> xbuf(static_cast<std::size_t>(n * 3));
  std::vector<float> ybuf(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const float a = static_cast<float>((i % 7) - 3) / 3.0f;
    const float b = static_cast<float>(((i * 3) % 11) - 5) / 4.0f;
    const float c = static_cast<float>(((i * 5) % 13) - 6) / 5.0f;
    xbuf[static_cast<std::size_t>(i * 3 + 0)] = a;
    xbuf[static_cast<std::size_t>(i * 3 + 1)] = b;
    xbuf[static_cast<std::size_t>(i * 3 + 2)] = c;
    ybuf[static_cast<std::size_t>(i)] = 0.7f * a - 1.1f * b + 0.4f * c + 0.2f;
  }
  const Series x = pluss::make_series(xbuf, {n, 3}, cfg);
  const Series y = pluss::make_series(ybuf, {n, 1}, cfg);
  return {x, y};
}

void seed_lstm_model_parameters_(TinyLSTMParityRegressor& model) {
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    *p = pluss::series::full_like(*p, 0.02f, ScalarType::Float32);
  }
}

void verify_lstm_seeded_parameters_(const TinyLSTMParityRegressor& model, Platform expected_platform) {
  auto params = model->parameters();
  std::unordered_set<const Series*> seen{};
  for (std::size_t i = 0; i < params.size(); ++i) {
    const Series* p = params[i];
    if (!p || !p->defined()) continue;
    if (!seen.insert(p).second) {
      throw std::runtime_error("lstm duplicate parameter pointer at index " + std::to_string(i));
    }
    if (p->platform() != expected_platform) {
      throw std::runtime_error(
          "lstm parameter platform mismatch at index " + std::to_string(i));
    }
    const Series cpu = p->to(pluss::pCPU, ScalarType::Float64);
    const double mn = cpu.min().item<double>();
    const double mx = cpu.max().item<double>();
    if (!(approx(mn, 0.02, 1e-6, 0.0) && approx(mx, 0.02, 1e-6, 0.0))) {
      throw std::runtime_error(
          "lstm seeded parameter mismatch at index " + std::to_string(i) +
          " min=" + std::to_string(mn) + " max=" + std::to_string(mx));
    }
  }
}

template <class ModelT>
void seed_model_parameters_(ModelT& model, float value) {
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    *p = pluss::series::full_like(*p, value, ScalarType::Float32);
  }
}

template <class ModelT>
void seed_model_parameters_ramp_(ModelT& model, float value, float ramp_step) {
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(p->platform());
    const Series idx =
        pluss::series::arange(0, p->numel(), 1, icfg).to(p->platform(), ScalarType::Float32).reshape(p->sizes());
    *p = pluss::series::full_like(*p, value, ScalarType::Float32).add(idx.mul(ramp_step));
  }
}

std::unique_ptr<pluss::nn::optim::Optimizer> make_linear_optimizer(
    LinearOptimizerKind kind,
    pluss::nn::Fusion& model) {
  switch (kind) {
    case LinearOptimizerKind::Adam: {
      pluss::nn::optim::AdamOptions opt{};
      opt.learning_rate = 5e-2;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      return std::make_unique<pluss::nn::optim::Adam>(*model, opt);
    }
    case LinearOptimizerKind::AdamW: {
      pluss::nn::optim::AdamWOptions opt{};
      opt.learning_rate = 5e-2;
      opt.weight_decay = 1e-2;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      return std::make_unique<pluss::nn::optim::AdamW>(*model, opt);
    }
    case LinearOptimizerKind::SGD: {
      pluss::nn::optim::SGDOptions opt{};
      opt.learning_rate = 1e-1;
      opt.momentum = 0.9;
      opt.nesterov = true;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      return std::make_unique<pluss::nn::optim::SGD>(*model, opt);
    }
    case LinearOptimizerKind::RMSprop: {
      pluss::nn::optim::RMSpropOptions opt{};
      opt.learning_rate = 1e-2;
      opt.momentum = 0.5;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      return std::make_unique<pluss::nn::optim::RMSprop>(*model, opt);
    }
    case LinearOptimizerKind::Adagrad: {
      pluss::nn::optim::AdagradOptions opt{};
      opt.learning_rate = 5e-2;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      return std::make_unique<pluss::nn::optim::Adagrad>(*model, opt);
    }
    case LinearOptimizerKind::NAdam: {
      pluss::nn::optim::NAdamOptions opt{};
      opt.learning_rate = 5e-2;
      opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
      return std::make_unique<pluss::nn::optim::NAdam>(*model, opt);
    }
    default:
      throw std::runtime_error("unsupported linear optimizer kind");
  }
}

TrainingTrace run_linear_training_trace(Platform platform, int epochs, LinearOptimizerKind kind) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_linear_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);
  const bool debug_trace = env_flag_is_true_("PLUSS_LINEAR_DEBUG_TRACE");
  const bool debug_grad = env_flag_is_true_("PLUSS_LINEAR_DEBUG_GRAD");
  const bool manual_step = env_flag_is_true_("PLUSS_LINEAR_MANUAL_STEP");
  const bool debug_arith = env_flag_is_true_("PLUSS_LINEAR_DEBUG_ARITH");

  pluss::nn::Fusion model(2, 1, true, cfg, 20260318ULL);
  model->weight() = pluss::make_series(std::vector<float>{0.10f, -0.20f}, {1, 2}, cfg);
  model->bias() = pluss::make_series(std::vector<float>{0.05f}, {1}, cfg);
  model->train(true);

  auto optimizer = make_linear_optimizer(kind, model);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    Series loss = pluss::series::undefined();
    if (manual_step) {
      optimizer->clear();
      loss = pluss::nn::losses::MSE(model->forward(x), y);
      loss.reverse();
      if (debug_grad) {
        const auto gw = model->weight().sensitivity().reshape({2}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto gb = model->bias().sensitivity().reshape({1}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        std::cout << "[linear-grad] platform=" << platform_name_(platform)
                  << " optimizer=" << linear_optimizer_name(kind)
                  << " epoch=" << (epoch + 1)
                  << " gw0=" << gw[0]
                  << " gw1=" << gw[1]
                  << " gb=" << gb[0] << "\n";
      }
      if (debug_arith && epoch == 0) {
        const Series p = model->weight().reshape({2});
        const Series g = model->weight().sensitivity().reshape({2});
        const Series lr = Series::full({}, 5e-2, p.scalar_type()).to(p.platform());
        const Series z = pluss::series::zeros_like(p, p.scalar_type());
        const Series z_mul = z.mul(Series::full({}, 0.9, p.scalar_type()).to(p.platform()));
        const Series one = Series::full({}, 1.0, p.scalar_type()).to(p.platform());
        const Series beta1 = Series::full({}, 0.9, p.scalar_type()).to(p.platform());
        const Series beta2 = Series::full({}, 0.999, p.scalar_type()).to(p.platform());
        const Series eps = Series::full({}, 1e-8, p.scalar_type()).to(p.platform());
        const Series b1_corr = Series::full({}, 0.1, p.scalar_type()).to(p.platform());
        const Series b2_corr = Series::full({}, 0.001, p.scalar_type()).to(p.platform());
        const Series one_minus_beta1 = one.sub(beta1);
        const Series one_minus_beta2 = one.sub(beta2);
        const Series m1_sim = z.mul(beta1).add(g.mul(one_minus_beta1));
        const Series m2_sim = z.mul(beta2).add(g.square().mul(one_minus_beta2));
        const Series first_hat_sim = m1_sim.div(b1_corr);
        const Series second_hat_sim = m2_sim.div(b2_corr);
        const Series den_sim = second_hat_sim.sqrt().add(eps);
        const Series delta_sim = first_hat_sim.div(den_sim).mul(lr);
        const Series next_sim = p.sub(delta_sim);
        const Series gb = model->bias().sensitivity().reshape({1});
        const Series zb = pluss::series::zeros_like(gb, gb.scalar_type());
        const Series one_b = Series::full({}, 1.0, gb.scalar_type()).to(gb.platform());
        const Series beta1_b = Series::full({}, 0.9, gb.scalar_type()).to(gb.platform());
        const Series one_minus_beta1_b = one_b.sub(beta1_b);
        const Series zbm = zb.mul(beta1_b);
        const Series m1b_sim = zbm.add(gb.mul(one_minus_beta1_b));
        const Series p_sub_g = p.sub(g);
        const Series g_mul_lr = g.mul(lr);
        const Series p_sub_glr = p.sub(g_mul_lr);
        const auto p_cpu = p.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto g_cpu = g.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto z_cpu = z.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto zm_cpu = z_mul.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto psg_cpu = p_sub_g.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto gml_cpu = g_mul_lr.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto psglr_cpu = p_sub_glr.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto omb1_cpu = one_minus_beta1.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto m1sim_cpu = m1_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto m2sim_cpu = m2_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto fhsim_cpu = first_hat_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto densim_cpu = den_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto dsim_cpu = delta_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto nsim_cpu = next_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto gb_cpu = gb.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto zbm_cpu = zbm.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const auto m1b_cpu = m1b_sim.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
        const double lr_cpu = lr.to(pluss::pCPU, ScalarType::Float64).item<double>();
        std::cout << "[linear-arith] platform=" << platform_name_(platform)
                  << " p0=" << p_cpu[0] << " p1=" << p_cpu[1]
                  << " g0=" << g_cpu[0] << " g1=" << g_cpu[1]
                  << " z0=" << z_cpu[0] << " z1=" << z_cpu[1]
                  << " z_mul0=" << zm_cpu[0] << " z_mul1=" << zm_cpu[1]
                  << " omb1_0=" << omb1_cpu[0] << " omb1_1=" << omb1_cpu[1]
                  << " m1sim0=" << m1sim_cpu[0] << " m1sim1=" << m1sim_cpu[1]
                  << " m2sim0=" << m2sim_cpu[0] << " m2sim1=" << m2sim_cpu[1]
                  << " fhsim0=" << fhsim_cpu[0] << " fhsim1=" << fhsim_cpu[1]
                  << " densim0=" << densim_cpu[0] << " densim1=" << densim_cpu[1]
                  << " dsim0=" << dsim_cpu[0] << " dsim1=" << dsim_cpu[1]
                  << " nsim0=" << nsim_cpu[0] << " nsim1=" << nsim_cpu[1]
                  << " gb=" << gb_cpu[0] << " zbm=" << zbm_cpu[0] << " m1b=" << m1b_cpu[0]
                  << " lr=" << lr_cpu
                  << " p_sub_g0=" << psg_cpu[0] << " p_sub_g1=" << psg_cpu[1]
                  << " g_mul_lr0=" << gml_cpu[0] << " g_mul_lr1=" << gml_cpu[1]
                  << " p_sub_glr0=" << psglr_cpu[0] << " p_sub_glr1=" << psglr_cpu[1]
                  << "\n";
      }
      optimizer->step();
    } else {
      loss = optimizer->step([&]() -> Series {
        return pluss::nn::losses::MSE(model->forward(x), y);
      });
    }
    const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
    trace.epoch_loss.push_back(loss_value);
    if (debug_trace) {
      const auto w_dbg = model->weight().reshape({2}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      const auto b_dbg = model->bias().reshape({1}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      std::cout << "[linear-debug] platform=" << platform_name_(platform)
                << " optimizer=" << linear_optimizer_name(kind)
                << " epoch=" << (epoch + 1)
                << " loss=" << loss_value
                << " w0=" << w_dbg[0]
                << " w1=" << w_dbg[1]
                << " b=" << b_dbg[0] << "\n";
    }
  }

  const auto w = model->weight().reshape({2}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto b = model->bias().reshape({1}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  trace.flat_parameters = {w[0], w[1], b[0]};
  return trace;
}

TrainingTrace run_linear_training_trace(Platform platform, int epochs) {
  return run_linear_training_trace(platform, epochs, LinearOptimizerKind::Adam);
}

TrainingTrace run_lstm_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_lstm_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);
  const bool debug_trace = env_flag_is_true_("PLUSS_LSTM_DEBUG_TRACE");
  const int debug_epochs = std::max(0, env_int_or_("PLUSS_LSTM_DEBUG_EPOCHS", 3));
  const bool debug_nonfinite = env_flag_is_true_("PLUSS_LSTM_DEBUG_NONFINITE");
  std::optional<pluss::subsp::math::diff::ReverseDebugScope> reverse_debug_scope{};
  if (debug_nonfinite) {
    reverse_debug_scope.emplace(true);
  }

  TinyLSTMParityRegressor model(cfg);
  model->train(true);
  model->to(platform, ScalarType::Float32);
  seed_lstm_model_parameters_(model);
  verify_lstm_seeded_parameters_(model, platform);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = (platform == pluss::pMPS) ? 2e-3 : 1e-2;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  if (platform == pluss::pMPS) {
    opt.clip.mode = pluss::nn::optim::GradientClipMode::ByNorm;
    opt.clip.threshold = 1.0;
  }
  pluss::nn::optim::Adam optimizer(*model, opt);
  pluss::nn::optim::StepDiagnosticsOptions diag{};
  diag.fail_if_no_sensitivity = true;
  optimizer.set_step_diagnostics(diag);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    try {
      optimizer.clear();
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm clear failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
    if (debug_trace && epoch < debug_epochs) {
      const auto named = model->named_parameters();
      for (const auto& named_param : named) {
        const Series* p = named_param.series;
        if (!p || !p->defined()) continue;
        std::cout << "[lstm-debug] platform=" << platform_name_(platform)
                  << " epoch=" << (epoch + 1)
                  << " pre-forward param=" << named_param.name
                  << " tracks=" << (p->tracks_slope() ? 1 : 0) << "\n";
      }
    }
    Series pred = pluss::series::undefined();
    try {
      pred = model->forward(x);
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm forward failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
    Series loss = pluss::series::undefined();
    try {
      loss = pluss::nn::losses::MSE(pred, y);
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm loss failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
    const double loss_f64 = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
    if (debug_trace && epoch < debug_epochs) {
      if (model->last_seq.defined()) {
        (void)model->last_seq.retain_grad();
      }
      for (Series* p : model->parameters()) {
        if (p && p->defined()) (void)p->retain_grad();
      }
    }
    try {
      loss.reverse();
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm backward failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
    if (debug_trace && epoch < debug_epochs) {
      if (model->last_seq.defined()) {
        const Series seq_cpu = model->last_seq.to(pluss::pCPU, ScalarType::Float64);
        const double seq_min = seq_cpu.min().item<double>();
        const double seq_max = seq_cpu.max().item<double>();
        const double seq_mean = seq_cpu.mean().item<double>();
        std::cout << "  last_seq tracks=" << (model->last_seq.tracks_slope() ? 1 : 0)
                  << " has_grad=" << (model->last_seq.has_sensitivity() ? 1 : 0)
                  << " min=" << seq_min
                  << " max=" << seq_max
                  << " mean=" << seq_mean;
        if (model->last_seq.has_sensitivity()) {
          const Series seq_grad_cpu = model->last_seq.sensitivity().to(pluss::pCPU, ScalarType::Float64);
          std::cout << " grad_l2=" << seq_grad_cpu.square().sum().sqrt().item<double>();
        }
        std::cout << "\n";
      }
      {
        const Series pred_cpu = pred.to(pluss::pCPU, ScalarType::Float64);
        std::cout << "  pred min=" << pred_cpu.min().item<double>()
                  << " max=" << pred_cpu.max().item<double>()
                  << " mean=" << pred_cpu.mean().item<double>() << "\n";
      }
      const auto named = model->named_parameters();
      std::cout << "[lstm-debug] platform=" << platform_name_(platform)
                << " epoch=" << (epoch + 1)
                << " loss=" << loss_f64 << "\n";
      for (const auto& named_param : named) {
        const Series* p = named_param.series;
        if (!p || !p->defined()) continue;
        if (!p->has_sensitivity()) {
          std::cout << "  param=" << named_param.name << " grad=missing\n";
          continue;
        }
        const Series grad_cpu = p->sensitivity().to(pluss::pCPU, ScalarType::Float64);
        const double g_l2 = grad_cpu.square().sum().sqrt().item<double>();
        const double g_max = grad_cpu.abs().max().item<double>();
        std::cout << "  param=" << named_param.name
                  << " grad_l2=" << g_l2
                  << " grad_max=" << g_max << "\n";
      }
    }
    try {
      optimizer.step();
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm optimizer step failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
    try {
      trace.epoch_loss.push_back(loss_f64);
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm loss materialize failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
  }

  double l1 = 0.0;
  double l2 = 0.0;
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    const Series cpu = p->to(pluss::pCPU, ScalarType::Float64);
    l1 += cpu.abs().sum().item<double>();
    l2 += cpu.square().sum().item<double>();
  }
  trace.flat_parameters = {l1, l2};
  return trace;
}

TrainingTrace run_rnn_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_rnn_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);

  TinyRNNParityRegressor model(cfg);
  model->train(true);
  model->to(platform, ScalarType::Float32);
  seed_model_parameters_(model, 0.02f);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 1e-2;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(*model, opt);
  pluss::nn::optim::StepDiagnosticsOptions diag{};
  diag.fail_if_no_sensitivity = true;
  optimizer.set_step_diagnostics(diag);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    optimizer.clear();
    const Series pred = model->forward(x);
    const Series loss = pluss::nn::losses::MSE(pred, y);
    loss.reverse();
    optimizer.step();
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
  }

  double l1 = 0.0;
  double l2 = 0.0;
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    const Series cpu = p->to(pluss::pCPU, ScalarType::Float64);
    l1 += cpu.abs().sum().item<double>();
    l2 += cpu.square().sum().item<double>();
  }
  trace.flat_parameters = {l1, l2};
  return trace;
}

TrainingTrace run_gru_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_rnn_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);
  const bool debug_trace = env_flag_is_true_("PLUSS_GRU_DEBUG_TRACE");
  const int debug_epochs = std::max(0, env_int_or_("PLUSS_GRU_DEBUG_EPOCHS", 3));

  TinyGRUParityRegressor model(cfg);
  model->train(true);
  model->to(platform, ScalarType::Float32);
  seed_model_parameters_(model, 0.02f);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 1e-2;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(*model, opt);
  pluss::nn::optim::StepDiagnosticsOptions diag{};
  diag.fail_if_no_sensitivity = true;
  optimizer.set_step_diagnostics(diag);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    optimizer.clear();
    const Series pred = model->forward(x);
    const Series loss = pluss::nn::losses::MSE(pred, y);
    std::vector<double> pre_param_l1{};
    if (debug_trace && epoch < debug_epochs) {
      auto params_dbg = model->parameters();
      pre_param_l1.reserve(params_dbg.size());
      for (Series* p : params_dbg) {
        if (!p || !p->defined()) {
          pre_param_l1.push_back(0.0);
          continue;
        }
        pre_param_l1.push_back(p->to(pluss::pCPU, ScalarType::Float64).abs().sum().item<double>());
      }
    }
    loss.reverse();
    if (debug_trace && epoch < debug_epochs) {
      std::cout << "[gru-debug] platform=" << platform_name_(platform)
                << " epoch=" << (epoch + 1)
                << " loss=" << loss.to(pluss::pCPU, ScalarType::Float64).item<double>() << "\n";
      const auto named = model->named_parameters();
      for (const auto& named_param : named) {
        const Series* p = named_param.series;
        if (!p || !p->defined()) continue;
        if (!p->has_sensitivity()) {
          std::cout << "  param=" << named_param.name << " grad=missing\n";
          continue;
        }
        const Series grad_cpu = p->sensitivity().to(pluss::pCPU, ScalarType::Float64);
        std::cout << "  param=" << named_param.name
                  << " grad_dtype=" << static_cast<int>(p->sensitivity().scalar_type())
                  << " grad_platform=" << static_cast<int>(p->sensitivity().platform())
                  << " grad_l2=" << grad_cpu.square().sum().sqrt().item<double>()
                  << " grad_max=" << grad_cpu.abs().max().item<double>()
                  << "\n";
      }
    }
    optimizer.step();
    if (debug_trace && epoch < debug_epochs) {
      auto params_dbg = model->parameters();
      for (std::size_t i = 0; i < params_dbg.size(); ++i) {
        Series* p = params_dbg[i];
        if (!p || !p->defined()) continue;
        const double post_l1 = p->to(pluss::pCPU, ScalarType::Float64).abs().sum().item<double>();
        const double delta = post_l1 - (i < pre_param_l1.size() ? pre_param_l1[i] : 0.0);
        std::cout << "  step_delta_l1[" << i << "]=" << delta << "\n";
      }
    }
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
  }

  double l1 = 0.0;
  double l2 = 0.0;
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    const Series cpu = p->to(pluss::pCPU, ScalarType::Float64);
    l1 += cpu.abs().sum().item<double>();
    l2 += cpu.square().sum().item<double>();
  }
  trace.flat_parameters = {l1, l2};
  return trace;
}

TrainingTrace run_schedule_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_linear_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);

  pluss::nn::Schedule model;
  model.append("encoder", pluss::nn::Fusion(2, 12, true, cfg, 41ULL))
      .append("activation", pluss::nn::Tanh())
      .append("head", pluss::nn::Fusion(12, 1, true, cfg, 43ULL));
  model->train(true);
  model->to(platform, ScalarType::Float32);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 2e-2;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(*model, opt);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      return pluss::nn::losses::MSE(model->forward(x), y);
    });
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
  }

  double l1 = 0.0;
  double l2 = 0.0;
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    const Series cpu = p->to(pluss::pCPU, ScalarType::Float64);
    l1 += cpu.abs().sum().item<double>();
    l2 += cpu.square().sum().item<double>();
  }
  trace.flat_parameters = {l1, l2};
  return trace;
}

TrainingTrace run_conv_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_conv_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);
  const bool debug_trace = env_flag_is_true_("PLUSS_CONV_DEBUG_TRACE");
  const int debug_epochs = std::max(0, env_int_or_("PLUSS_CONV_DEBUG_EPOCHS", 4));
  if (debug_trace) {
    const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
    const Series idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2}, {3}, icfg);
    const Series idx_from_cpu = pluss::make_series(std::vector<std::int64_t>{0, 1, 2}, {3}, Config{}.with_dtype(ScalarType::Int64))
                                    .to(platform, ScalarType::Int64);
    const Series base = x.to(pluss::pCPU, ScalarType::Float64).flatten(0, -1);
    const Series pick = x.index_select(2, idx).to(pluss::pCPU, ScalarType::Float64).flatten(0, -1);
    const Series unf = x.unfold(2, 3, 1).to(pluss::pCPU, ScalarType::Float64).flatten(0, -1);
    const auto idx_v = idx.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
    const auto idx_from_cpu_v = idx_from_cpu.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
    const auto base_v = base.to_vec<double>();
    const auto pick_v = pick.to_vec<double>();
    const auto unf_v = unf.to_vec<double>();
    std::cout << "[conv-probe] platform=" << platform_name_(platform)
              << " idx_native=[" << idx_v[0] << "," << idx_v[1] << "," << idx_v[2] << "]"
              << " idx_from_cpu=[" << idx_from_cpu_v[0] << "," << idx_from_cpu_v[1] << "," << idx_from_cpu_v[2] << "]"
              << " base=[" << base_v[0] << "," << base_v[1] << "," << base_v[2] << "]"
              << " pick=[" << pick_v[0] << "," << pick_v[1] << "," << pick_v[2] << "]"
              << " unfold_w0=[" << unf_v[0] << "," << unf_v[1] << "," << unf_v[2] << "]\n";
  }

  pluss::nn::Conv1d model(pluss::nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = 3,
      .stride = 1,
      .padding = 1,
      .dilation = 1,
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 47ULL,
  });
  model->train(true);
  model->to(platform, ScalarType::Float32);
  seed_model_parameters_(model, 0.02f);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 1e-2;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(*model, opt);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    Series loss = pluss::series::undefined();
    if (debug_trace && epoch < debug_epochs) {
      optimizer.clear();
      const Series pred = model->forward(x);
      loss = pluss::nn::losses::MSE(pred, y);
      const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
      loss.reverse();
      const Series gw = model->weight().sensitivity().to(pluss::pCPU, ScalarType::Float64).reshape({model->weight().numel()});
      const Series gb = model->bias().sensitivity().to(pluss::pCPU, ScalarType::Float64).reshape({model->bias().numel()});
      const Series w = model->weight().to(pluss::pCPU, ScalarType::Float64).reshape({model->weight().numel()});
      const Series b = model->bias().to(pluss::pCPU, ScalarType::Float64).reshape({model->bias().numel()});
      const auto wv = w.to_vec<double>();
      const auto gwv = gw.to_vec<double>();
      std::cout << "[conv-debug] platform=" << platform_name_(platform)
                << " epoch=" << (epoch + 1)
                << " loss=" << loss_value
                << " w0=" << wv[0]
                << " w1=" << (wv.size() > 1 ? wv[1] : 0.0)
                << " w2=" << (wv.size() > 2 ? wv[2] : 0.0)
                << " b0=" << b.to_vec<double>()[0]
                << " gw0=" << gwv[0]
                << " gw1=" << (gwv.size() > 1 ? gwv[1] : 0.0)
                << " gw2=" << (gwv.size() > 2 ? gwv[2] : 0.0)
                << " gb0=" << gb.to_vec<double>()[0]
                << " gw_l2=" << gw.square().sum().sqrt().item<double>()
                << " gb_l2=" << gb.square().sum().sqrt().item<double>()
                << "\n";
      optimizer.step();
    } else {
      loss = optimizer.step([&]() -> Series {
        return pluss::nn::losses::MSE(model->forward(x), y);
      });
    }
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
  }

  const Series w_cpu = model->weight().to(pluss::pCPU, ScalarType::Float64);
  const Series b_cpu = model->bias().to(pluss::pCPU, ScalarType::Float64);
  trace.flat_parameters = {
      w_cpu.abs().sum().item<double>() + b_cpu.abs().sum().item<double>(),
      w_cpu.square().sum().item<double>() + b_cpu.square().sum().item<double>(),
  };
  return trace;
}

TrainingTrace run_transformer_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [src_cpu, tgt_cpu] = make_transformer_autoencode_data();
  const Series src = src_cpu.to(platform, ScalarType::Float32);
  const Series tgt = tgt_cpu.to(platform, ScalarType::Float32);

  pluss::nn::Transformer model(pluss::nn::TransformerOptions{
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
      .seed = 53ULL,
      .training = true,
  });
  model->train(true);
  model->to(platform, ScalarType::Float32);
  seed_model_parameters_(model, 0.02f);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = 5e-3;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(*model, opt);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    const Series loss = optimizer.step([&]() -> Series {
      return pluss::nn::losses::MSE(model->forward(src, tgt), tgt);
    });
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
  }

  double l1 = 0.0;
  double l2 = 0.0;
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    const Series cpu = p->to(pluss::pCPU, ScalarType::Float64);
    l1 += cpu.abs().sum().item<double>();
    l2 += cpu.square().sum().item<double>();
  }
  trace.flat_parameters = {l1, l2};
  return trace;
}

TrainingTrace run_custom_objective_training_trace(Platform platform, int epochs) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
  const auto [x_cpu, y_cpu] = make_custom_objective_data();
  const Series x = x_cpu.to(platform, ScalarType::Float32);
  const Series y = y_cpu.to(platform, ScalarType::Float32);

  Series w = pluss::make_series(std::vector<float>{0.2f, -0.1f, 0.05f}, {3}, cfg);
  w.tracks_slope_(true);

  pluss::nn::optim::AdamOptions opt{};
  opt.learning_rate = env_double_or_(
      "PLUSS_OBJECTIVE_LR",
      (platform == pluss::pMPS) ? 5e-4 : 2e-2);
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
  pluss::nn::optim::Adam optimizer(std::vector<Series*>{&w}, opt);

  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
  const Series idx = pluss::make_series(std::vector<std::int64_t>{2, 0}, {2}, icfg);
  const bool debug_trace = env_flag_is_true_("PLUSS_OBJECTIVE_DEBUG_TRACE");
  const int debug_epochs = std::max(0, env_int_or_("PLUSS_OBJECTIVE_DEBUG_EPOCHS", 6));

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    const auto objective = [&]() -> Series {
      const Series pred = x.matmul(w.reshape({3, 1})).reshape({x.sizes()[0], 1});
      const Series resid = pred.sub(y);
      const Series mse = resid.square().mean();
      const Series reg1 = w.sin().square().mean().mul(0.10);
      const Series reg2 = w.abs().mean().mul(0.05);
      const Series reg3 = w.take(idx).square().sum().mul(0.01);
      if (debug_trace && epoch == 0) {
        const Series x_device_roundtrip =
            x.add(0.0).to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()});
        const Series pred_from_cpu_copy =
            x.to(pluss::pCPU, ScalarType::Float64)
                .matmul(w.to(pluss::pCPU, ScalarType::Float64).reshape({3, 1}))
                .reshape({x.sizes()[0], 1});
        const auto pv = pred.to(pluss::pCPU, ScalarType::Float64).flatten(0, -1).to_vec<double>();
        const auto pvc = pred_from_cpu_copy.to(pluss::pCPU, ScalarType::Float64).flatten(0, -1).to_vec<double>();
        const auto yv = y.to(pluss::pCPU, ScalarType::Float64).flatten(0, -1).to_vec<double>();
        const auto xv = x.to(pluss::pCPU, ScalarType::Float64).flatten(0, -1).to_vec<double>();
        const auto xdv = x_device_roundtrip.to_vec<double>();
        const auto xsel = x.select(1, 0).to(pluss::pCPU, ScalarType::Float64).reshape({x.sizes()[0]}).to_vec<double>();
        const auto wrv = w.reshape({3, 1}).to(pluss::pCPU, ScalarType::Float64).reshape({3}).to_vec<double>();
        const auto wrcv = w.reshape({3, 1}).contiguous().to(pluss::pCPU, ScalarType::Float64).reshape({3}).to_vec<double>();
        std::cout << "[objective-terms] platform=" << platform_name_(platform)
                  << " mse=" << mse.to(pluss::pCPU, ScalarType::Float64).item<double>()
                  << " reg1=" << reg1.to(pluss::pCPU, ScalarType::Float64).item<double>()
                  << " reg2=" << reg2.to(pluss::pCPU, ScalarType::Float64).item<double>()
                  << " reg3=" << reg3.to(pluss::pCPU, ScalarType::Float64).item<double>()
                  << " x0=" << xv[0] << " x1=" << xv[1] << " x2=" << xv[2]
                  << " xd0=" << xdv[0] << " xd1=" << xdv[1] << " xd2=" << xdv[2]
                  << " xs0=" << xsel[0] << " xs1=" << xsel[1] << " xs2=" << xsel[2]
                  << " wr0=" << wrv[0] << " wr1=" << wrv[1] << " wr2=" << wrv[2]
                  << " wrc0=" << wrcv[0] << " wrc1=" << wrcv[1] << " wrc2=" << wrcv[2]
                  << " pred0=" << pv[0] << " pred1=" << pv[1] << " pred2=" << pv[2]
                  << " pred_cpu_copy0=" << pvc[0] << " pred_cpu_copy1=" << pvc[1] << " pred_cpu_copy2=" << pvc[2]
                  << " y0=" << yv[0] << " y1=" << yv[1] << " y2=" << yv[2]
                  << "\n";
      }
      return mse.add(reg1).add(reg2).add(reg3);
    };
    optimizer.clear();
    Series loss = objective();
    const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
    loss.reverse();
    if (debug_trace && epoch < debug_epochs) {
      const auto grad = w.sensitivity().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      const auto val = w.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
      std::cout << "[objective-debug] platform=" << platform_name_(platform)
                << " epoch=" << (epoch + 1)
                << " loss=" << loss_value
                << " w=(" << val[0] << "," << val[1] << "," << val[2] << ")"
                << " g=(" << grad[0] << "," << grad[1] << "," << grad[2] << ")"
                << "\n";
    }
    optimizer.step();
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
  }

  const auto wv = w.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  trace.flat_parameters = wv;
  return trace;
}

bool trace_descends(const TrainingTrace& trace) {
  if (trace.epoch_loss.empty()) return false;
  return trace.epoch_loss.back() < trace.epoch_loss.front();
}

bool compare_traces(const TrainingTrace& lhs,
                    const TrainingTrace& rhs,
                    double loss_atol,
                    double loss_rtol,
                    double param_atol,
                    double param_rtol,
                    std::ostream* err = nullptr) {
  if (lhs.epoch_loss.size() != rhs.epoch_loss.size()) {
    if (err) *err << "trace length mismatch\n";
    return false;
  }
  if (lhs.flat_parameters.size() != rhs.flat_parameters.size()) {
    if (err) *err << "parameter vector length mismatch\n";
    return false;
  }
  for (std::size_t i = 0; i < lhs.epoch_loss.size(); ++i) {
    if (!approx(lhs.epoch_loss[i], rhs.epoch_loss[i], loss_atol, loss_rtol)) {
      if (err) {
        *err << "epoch loss mismatch at epoch " << (i + 1)
             << " lhs=" << lhs.epoch_loss[i]
             << " rhs=" << rhs.epoch_loss[i] << "\n";
      }
      return false;
    }
  }
  for (std::size_t i = 0; i < lhs.flat_parameters.size(); ++i) {
    if (!approx(lhs.flat_parameters[i], rhs.flat_parameters[i], param_atol, param_rtol)) {
      if (err) {
        *err << "parameter mismatch at index " << i
             << " lhs=" << lhs.flat_parameters[i]
             << " rhs=" << rhs.flat_parameters[i] << "\n";
      }
      return false;
    }
  }
  return true;
}

std::vector<double> load_torch_reference_losses_csv_(const char* path) {
  std::vector<double> losses{};
  std::ifstream in(path);
  if (!in.good()) return losses;

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    if (line.find("epoch") != std::string::npos && line.find("loss") != std::string::npos) continue;
    std::stringstream ss(line);
    std::string col_epoch;
    std::string col_loss;
    if (!std::getline(ss, col_epoch, ',')) continue;
    if (!std::getline(ss, col_loss, ',')) continue;
    try {
      losses.push_back(std::stod(col_loss));
    } catch (...) {
      continue;
    }
  }
  return losses;
}

bool compare_losses_to_torch_reference_(const std::vector<double>& losses,
                                        const char* env_var_name,
                                        double atol,
                                        double rtol,
                                        std::ostream* err = nullptr) {
  const char* torch_ref = std::getenv(env_var_name);
  if (!torch_ref) return true;

  const auto torch_losses = load_torch_reference_losses_csv_(torch_ref);
  if (torch_losses.empty()) return true;
  if (torch_losses.size() != losses.size()) {
    if (err) {
      *err << "Torch reference epoch count mismatch for " << env_var_name
           << " (torch=" << torch_losses.size() << ", pluss=" << losses.size() << ")\n";
    }
    return false;
  }
  for (std::size_t i = 0; i < torch_losses.size(); ++i) {
    if (!approx(losses[i], torch_losses[i], atol, rtol)) {
      if (err) {
        *err << "Torch reference mismatch for " << env_var_name
             << " at epoch " << (i + 1)
             << " pluss=" << losses[i]
             << " torch=" << torch_losses[i] << "\n";
      }
      return false;
    }
  }
  return true;
}

bool compare_losses_to_torch_reference_any_(const std::vector<double>& losses,
                                            std::initializer_list<const char*> env_var_names,
                                            double atol,
                                            double rtol,
                                            std::ostream* err = nullptr) {
  const char* selected = nullptr;
  const char* selected_name = nullptr;
  for (const char* name : env_var_names) {
    if (!name) continue;
    const char* path = std::getenv(name);
    if (!path || *path == '\0') continue;
    selected = path;
    selected_name = name;
    break;
  }
  if (!selected || !selected_name) return true;

  const auto torch_losses = load_torch_reference_losses_csv_(selected);
  if (torch_losses.empty()) return true;
  if (torch_losses.size() != losses.size()) {
    if (err) {
      *err << "Torch reference epoch count mismatch for " << selected_name
           << " (torch=" << torch_losses.size() << ", pluss=" << losses.size() << ")\n";
    }
    return false;
  }
  for (std::size_t i = 0; i < torch_losses.size(); ++i) {
    if (!approx(losses[i], torch_losses[i], atol, rtol)) {
      if (err) {
        *err << "Torch reference mismatch for " << selected_name
             << " at epoch " << (i + 1)
             << " pluss=" << losses[i]
             << " torch=" << torch_losses[i] << "\n";
      }
      return false;
    }
  }
  return true;
}

bool run_training_parity_checks(std::ostream* err = nullptr) {
  auto guarded = [&](const char* stage, auto&& fn) -> bool {
    try {
      return fn();
    } catch (const std::exception& ex) {
      if (err) *err << stage << " threw: " << ex.what() << "\n";
      return false;
    } catch (...) {
      if (err) *err << stage << " threw unknown exception\n";
      return false;
    }
  };

  const ParityBackendMode mode = parity_backend_mode_();
  const bool want_cpu = mode == ParityBackendMode::All || mode == ParityBackendMode::CPU;
  const bool want_mps = mode == ParityBackendMode::All || mode == ParityBackendMode::MPS;
  const bool want_cuda = mode == ParityBackendMode::All || mode == ParityBackendMode::CUDA;

  const bool run_cpu = want_cpu;
  const bool run_mps = want_mps && pluss::mps::is_ready();
  const bool run_cuda = want_cuda && pluss::cuda::is_ready();
  if (mode == ParityBackendMode::MPS && !run_mps) {
    if (err) *err << "MPS backend requested but runtime is unavailable\n";
    return false;
  }
  if (mode == ParityBackendMode::CUDA && !run_cuda) {
    if (err) *err << "CUDA backend requested but runtime is unavailable\n";
    return false;
  }
  if (!run_cpu && !run_mps && !run_cuda) {
    if (err) *err << "No requested training parity backend is available\n";
    return false;
  }

  std::optional<ScopedEnvVar> mps_fallback_off{};
  std::optional<ScopedEnvVar> mps_native_take_put{};
  std::optional<ScopedEnvVar> mps_native_sort{};
  if (run_mps) {
    mps_fallback_off.emplace("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
    mps_native_take_put.emplace("PLUSS_MPS_NATIVE_TAKE_PUT", "1");
    mps_native_sort.emplace("PLUSS_MPS_NATIVE_SORT_SELECTION", "1");
  }
  std::optional<ScopedEnvVar> cuda_bridge_off{};
  if (run_cuda) {
    cuda_bridge_off.emplace("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  }

  constexpr int kLinearEpochs = 32;
  constexpr int kLstmEpochs = 36;
  constexpr int kRnnEpochs = 34;
  constexpr int kGruEpochs = 34;
  constexpr int kScheduleEpochs = 30;
  constexpr int kConvEpochs = 28;
  constexpr int kTransformerEpochs = 20;
  constexpr int kObjectiveEpochs = 30;

  auto check_trace = [&](const char* label, const TrainingTrace& trace) -> bool {
    if (!trace_descends(trace)) {
      if (err) {
        *err << label << " training should reduce loss (first=" << trace.epoch_loss.front()
             << ", last=" << trace.epoch_loss.back() << ")\n";
      }
      return false;
    }
    if (!trace_is_finite(trace)) {
      if (err) *err << label << " trace has non-finite values\n";
      return false;
    }
    return true;
  };

  const char* model_raw = std::getenv("PLUSS_PARITY_MODEL");
  if (model_raw != nullptr && *model_raw != '\0') {
    std::string model(model_raw);
    for (char& c : model) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (model != "all") {
      auto run_single_model = [&](Platform platform, const char* tag) -> bool {
        TrainingTrace trace{};
        const std::string lane = std::string(tag) + "_" + model;
        const bool ok = guarded(lane.c_str(), [&]() {
          if (model == "linear") {
            trace = run_linear_training_trace(platform, kLinearEpochs);
          } else if (model == "lstm") {
            trace = run_lstm_training_trace(platform, kLstmEpochs);
          } else if (model == "rnn") {
            trace = run_rnn_training_trace(platform, kRnnEpochs);
          } else if (model == "gru") {
            trace = run_gru_training_trace(platform, kGruEpochs);
          } else if (model == "schedule") {
            trace = run_schedule_training_trace(platform, kScheduleEpochs);
          } else if (model == "conv") {
            trace = run_conv_training_trace(platform, kConvEpochs);
          } else if (model == "transformer") {
            trace = run_transformer_training_trace(platform, kTransformerEpochs);
          } else if (model == "objective") {
            trace = run_custom_objective_training_trace(platform, kObjectiveEpochs);
          } else {
            if (err) {
              *err << "unknown PLUSS_PARITY_MODEL='" << model
                   << "' (expected all|linear|lstm|rnn|gru|schedule|conv|transformer|objective)\n";
            }
            return false;
          }
          return true;
        });
        if (!ok) return false;
        return check_trace((std::string(tag) + " " + model).c_str(), trace);
      };

      if (run_cpu && !run_single_model(pluss::pCPU, "cpu")) return false;
      if (run_mps && !run_single_model(pluss::pMPS, "mps")) return false;
      if (run_cuda && !run_single_model(pluss::pCUDA, "cuda")) return false;
      return true;
    }
  }

  auto check_replay = [&](const char* lane,
                          auto&& rerun,
                          const TrainingTrace& baseline,
                          double loss_atol,
                          double loss_rtol,
                          double param_atol,
                          double param_rtol) -> bool {
    TrainingTrace replay{};
    if (!guarded(lane, [&]() {
          replay = rerun();
          return true;
        })) {
      return false;
    }
    if (!compare_traces(baseline, replay, loss_atol, loss_rtol, param_atol, param_rtol, err)) {
      if (err) *err << "determinism mismatch for " << lane << "\n";
      return false;
    }
    return true;
  };

  struct BackendSuite {
    TrainingTrace linear{};
    TrainingTrace lstm{};
    TrainingTrace rnn{};
    TrainingTrace gru{};
    TrainingTrace schedule{};
    TrainingTrace conv{};
    TrainingTrace transformer{};
    TrainingTrace objective{};
  };

  auto run_suite = [&](Platform platform, const char* tag, BackendSuite* suite) -> bool {
    if (!guarded((std::string(tag) + "_linear").c_str(), [&]() {
          suite->linear = run_linear_training_trace(platform, kLinearEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " linear").c_str(), suite->linear)) return false;

    if (!guarded((std::string(tag) + "_lstm").c_str(), [&]() {
          suite->lstm = run_lstm_training_trace(platform, kLstmEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " lstm").c_str(), suite->lstm)) return false;

    if (!guarded((std::string(tag) + "_rnn").c_str(), [&]() {
          suite->rnn = run_rnn_training_trace(platform, kRnnEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " rnn").c_str(), suite->rnn)) return false;

    if (!guarded((std::string(tag) + "_gru").c_str(), [&]() {
          suite->gru = run_gru_training_trace(platform, kGruEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " gru").c_str(), suite->gru)) return false;

    if (!guarded((std::string(tag) + "_schedule").c_str(), [&]() {
          suite->schedule = run_schedule_training_trace(platform, kScheduleEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " schedule").c_str(), suite->schedule)) return false;

    if (!guarded((std::string(tag) + "_conv").c_str(), [&]() {
          suite->conv = run_conv_training_trace(platform, kConvEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " conv").c_str(), suite->conv)) return false;

    if (!guarded((std::string(tag) + "_transformer").c_str(), [&]() {
          suite->transformer = run_transformer_training_trace(platform, kTransformerEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " transformer").c_str(), suite->transformer)) return false;

    if (!guarded((std::string(tag) + "_objective").c_str(), [&]() {
          suite->objective = run_custom_objective_training_trace(platform, kObjectiveEpochs);
          return true;
        })) return false;
    if (!check_trace((std::string(tag) + " objective").c_str(), suite->objective)) return false;

    const bool cpu_like = platform == pluss::pCPU;
    const double replay_loss_atol = cpu_like ? 1e-7 : 5e-4;
    const double replay_loss_rtol = cpu_like ? 1e-6 : 5e-3;
    const double replay_param_atol = cpu_like ? 1e-6 : 5e-4;
    const double replay_param_rtol = cpu_like ? 1e-5 : 5e-3;
    if (!check_replay(
            (std::string(tag) + "_replay_linear").c_str(),
            [&]() { return run_linear_training_trace(platform, kLinearEpochs); },
            suite->linear,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_lstm").c_str(),
            [&]() { return run_lstm_training_trace(platform, kLstmEpochs); },
            suite->lstm,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_rnn").c_str(),
            [&]() { return run_rnn_training_trace(platform, kRnnEpochs); },
            suite->rnn,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_gru").c_str(),
            [&]() { return run_gru_training_trace(platform, kGruEpochs); },
            suite->gru,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_schedule").c_str(),
            [&]() { return run_schedule_training_trace(platform, kScheduleEpochs); },
            suite->schedule,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_conv").c_str(),
            [&]() { return run_conv_training_trace(platform, kConvEpochs); },
            suite->conv,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_transformer").c_str(),
            [&]() { return run_transformer_training_trace(platform, kTransformerEpochs); },
            suite->transformer,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    if (!check_replay(
            (std::string(tag) + "_replay_objective").c_str(),
            [&]() { return run_custom_objective_training_trace(platform, kObjectiveEpochs); },
            suite->objective,
            replay_loss_atol,
            replay_loss_rtol,
            replay_param_atol,
            replay_param_rtol)) return false;
    return true;
  };

  auto compare_suite_to_torch = [&](Platform platform, const BackendSuite& suite) -> bool {
    const bool is_mps = platform == pluss::pMPS;
    const bool is_cuda = platform == pluss::pCUDA;
    if (!compare_losses_to_torch_reference_any_(
            suite.linear.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_LINEAR_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_LINEAR_REF_CSV" : nullptr,
             "PLUSS_TORCH_LINEAR_REF_CSV"},
            2e-3,
            0.25,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.lstm.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_LSTM_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_LSTM_REF_CSV" : nullptr,
             "PLUSS_TORCH_LSTM_REF_CSV"},
            1.2e-1,
            0.75,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.rnn.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_RNN_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_RNN_REF_CSV" : nullptr,
             "PLUSS_TORCH_RNN_REF_CSV"},
            1.5e-1,
            0.80,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.gru.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_GRU_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_GRU_REF_CSV" : nullptr,
             "PLUSS_TORCH_GRU_REF_CSV"},
            1.5e-1,
            0.80,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.schedule.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_SCHEDULE_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_SCHEDULE_REF_CSV" : nullptr,
             "PLUSS_TORCH_SCHEDULE_REF_CSV"},
            9e-2,
            1.00,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.conv.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_CONV_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_CONV_REF_CSV" : nullptr,
             "PLUSS_TORCH_CONV_REF_CSV"},
            4e-2,
            0.60,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.transformer.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_TRANSFORMER_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_TRANSFORMER_REF_CSV" : nullptr,
             "PLUSS_TORCH_TRANSFORMER_REF_CSV"},
            6e-2,
            0.80,
            err)) return false;
    if (!compare_losses_to_torch_reference_any_(
            suite.objective.epoch_loss,
            {is_mps ? "PLUSS_TORCH_MPS_OBJECTIVE_REF_CSV" : nullptr,
             is_cuda ? "PLUSS_TORCH_CUDA_OBJECTIVE_REF_CSV" : nullptr,
             "PLUSS_TORCH_OBJECTIVE_REF_CSV"},
            8e-3,
            0.20,
            err)) return false;
    return true;
  };

  std::optional<BackendSuite> cpu_suite{};
  std::optional<BackendSuite> mps_suite{};
  std::optional<BackendSuite> cuda_suite{};
  if (run_cpu) {
    cpu_suite.emplace();
    if (!run_suite(pluss::pCPU, "cpu", &*cpu_suite)) return false;
    if (!compare_suite_to_torch(pluss::pCPU, *cpu_suite)) return false;
  }
  if (run_mps) {
    mps_suite.emplace();
    if (!run_suite(pluss::pMPS, "mps", &*mps_suite)) return false;
    if (!compare_suite_to_torch(pluss::pMPS, *mps_suite)) return false;
  }
  if (run_cuda) {
    cuda_suite.emplace();
    if (!run_suite(pluss::pCUDA, "cuda", &*cuda_suite)) return false;
    if (!compare_suite_to_torch(pluss::pCUDA, *cuda_suite)) return false;
  }

  const std::vector<LinearOptimizerKind> matrix_kinds{
      LinearOptimizerKind::Adam,
      LinearOptimizerKind::AdamW,
      LinearOptimizerKind::SGD,
      LinearOptimizerKind::RMSprop,
      LinearOptimizerKind::Adagrad,
      LinearOptimizerKind::NAdam,
  };
  constexpr int kLinearMatrixEpochs = 24;
  for (const auto kind : matrix_kinds) {
    const char* name = linear_optimizer_name(kind);
    const auto [atol, rtol] = torch_linear_loss_tolerance(kind);
    if (run_cpu) {
      TrainingTrace cpu{};
      if (!guarded((std::string("cpu_linear_matrix_") + name).c_str(), [&]() {
            cpu = run_linear_training_trace(pluss::pCPU, kLinearMatrixEpochs, kind);
            return true;
          })) return false;
      if (!check_trace((std::string("CPU ") + name).c_str(), cpu)) return false;
      if (!check_replay(
              (std::string("cpu_linear_matrix_replay_") + name).c_str(),
              [&]() { return run_linear_training_trace(pluss::pCPU, kLinearMatrixEpochs, kind); },
              cpu,
              1e-7,
              1e-6,
              1e-7,
              1e-6)) return false;
      if (!compare_losses_to_torch_reference_any_(
              cpu.epoch_loss,
              {torch_env_for_linear_optimizer(kind), "PLUSS_TORCH_LINEAR_REF_CSV"},
              atol,
              rtol,
              err)) return false;
    }
    if (run_mps) {
      TrainingTrace mps{};
      if (!guarded((std::string("mps_linear_matrix_") + name).c_str(), [&]() {
            mps = run_linear_training_trace(pluss::pMPS, kLinearMatrixEpochs, kind);
            return true;
          })) return false;
      if (!check_trace((std::string("MPS ") + name).c_str(), mps)) return false;
      if (!check_replay(
              (std::string("mps_linear_matrix_replay_") + name).c_str(),
              [&]() { return run_linear_training_trace(pluss::pMPS, kLinearMatrixEpochs, kind); },
              mps,
              5e-4,
              5e-3,
              5e-4,
              5e-3)) return false;
      if (!compare_losses_to_torch_reference_any_(
              mps.epoch_loss,
              {torch_env_for_linear_optimizer_mps(kind),
               torch_env_for_linear_optimizer(kind),
               "PLUSS_TORCH_LINEAR_REF_CSV"},
              atol,
              rtol,
              err)) return false;
    }
    if (run_cuda) {
      TrainingTrace cuda{};
      if (!guarded((std::string("cuda_linear_matrix_") + name).c_str(), [&]() {
            cuda = run_linear_training_trace(pluss::pCUDA, kLinearMatrixEpochs, kind);
            return true;
          })) return false;
      if (!check_trace((std::string("CUDA ") + name).c_str(), cuda)) return false;
      if (!check_replay(
              (std::string("cuda_linear_matrix_replay_") + name).c_str(),
              [&]() { return run_linear_training_trace(pluss::pCUDA, kLinearMatrixEpochs, kind); },
              cuda,
              5e-4,
              5e-3,
              5e-4,
              5e-3)) return false;
      if (!compare_losses_to_torch_reference_any_(
              cuda.epoch_loss,
              {torch_env_for_linear_optimizer_cuda(kind),
               torch_env_for_linear_optimizer(kind),
               "PLUSS_TORCH_LINEAR_REF_CSV"},
              atol,
              rtol,
              err)) return false;
    }
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn training trajectory parity across backends") {
  REQUIRE(run_training_parity_checks(&std::cerr));
}
#else
int main() {
  if (!run_training_parity_checks(&std::cerr)) return 1;
  std::cout << "nn_training_parity_test passed\n";
  return 0;
}
#endif
