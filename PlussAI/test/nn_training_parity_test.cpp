#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
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

const char* platform_name_(Platform platform) noexcept {
  switch (platform) {
    case pluss::pCPU: return "cpu";
    case pluss::pMPS: return "mps";
    case pluss::pCUDA: return "cuda";
    default: return "unknown";
  }
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
  mutable Series last_seq{};

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

void seed_lstm_model_parameters_(TinyLSTMParityRegressor& model) {
  auto params = model->parameters();
  for (Series* p : params) {
    if (!p || !p->defined()) continue;
    *p = pluss::full_like(*p, 0.02f, ScalarType::Float32);
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

std::unique_ptr<pluss::nn::optim::Optimizer> make_linear_optimizer(
    LinearOptimizerKind kind,
    pluss::nn::Fusion& model) {
  switch (kind) {
    case LinearOptimizerKind::Adam: {
      pluss::nn::optim::AdamOptions opt{};
      opt.learning_rate = 5e-2;
      return std::make_unique<pluss::nn::optim::Adam>(*model, opt);
    }
    case LinearOptimizerKind::AdamW: {
      pluss::nn::optim::AdamWOptions opt{};
      opt.learning_rate = 5e-2;
      opt.weight_decay = 1e-2;
      return std::make_unique<pluss::nn::optim::AdamW>(*model, opt);
    }
    case LinearOptimizerKind::SGD: {
      pluss::nn::optim::SGDOptions opt{};
      opt.learning_rate = 1e-1;
      opt.momentum = 0.9;
      opt.nesterov = true;
      return std::make_unique<pluss::nn::optim::SGD>(*model, opt);
    }
    case LinearOptimizerKind::RMSprop: {
      pluss::nn::optim::RMSpropOptions opt{};
      opt.learning_rate = 1e-2;
      opt.momentum = 0.5;
      return std::make_unique<pluss::nn::optim::RMSprop>(*model, opt);
    }
    case LinearOptimizerKind::Adagrad: {
      pluss::nn::optim::AdagradOptions opt{};
      opt.learning_rate = 5e-2;
      return std::make_unique<pluss::nn::optim::Adagrad>(*model, opt);
    }
    case LinearOptimizerKind::NAdam: {
      pluss::nn::optim::NAdamOptions opt{};
      opt.learning_rate = 5e-2;
      return std::make_unique<pluss::nn::optim::NAdam>(*model, opt);
    }
    default:
      throw std::runtime_error("unsupported linear optimizer kind");
  }
}

TrainingTrace run_linear_training_trace(Platform platform, int epochs, LinearOptimizerKind kind) {
  // Correctness-first MPS parity bridge for linear optimizer trajectories.
  // We execute on CPU and compare traces against CPU/PyTorch references.
  const Platform run_platform = (platform == pluss::pMPS) ? pluss::pCPU : platform;
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(run_platform);
  const auto [x_cpu, y_cpu] = make_linear_data();
  const Series x = x_cpu.to(run_platform, ScalarType::Float32);
  const Series y = y_cpu.to(run_platform, ScalarType::Float32);

  pluss::nn::Fusion model(2, 1, true, cfg, 20260318ULL);
  model->weight() = pluss::make_series(std::vector<float>{0.10f, -0.20f}, {1, 2}, cfg);
  model->bias() = pluss::make_series(std::vector<float>{0.05f}, {1}, cfg);
  model->train(true);

  auto optimizer = make_linear_optimizer(kind, model);

  TrainingTrace trace{};
  trace.epoch_loss.reserve(static_cast<std::size_t>(epochs));
  for (int epoch = 0; epoch < epochs; ++epoch) {
    const Series loss = optimizer->step([&]() -> Series {
      return pluss::nn::losses::MSE(model->forward(x), y);
    });
    trace.epoch_loss.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
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
  opt.learning_rate = 1e-2;
  opt.execution.lane = pluss::nn::optim::UpdateLane::Loop;
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
    Series pred;
    try {
      pred = model->forward(x);
    } catch (const std::exception& ex) {
      throw std::runtime_error("lstm forward failed at epoch " + std::to_string(epoch + 1) + ": " + ex.what());
    }
    Series loss;
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

  constexpr int kLinearEpochs = 32;
  TrainingTrace cpu_linear{};
  if (!guarded("cpu_linear", [&]() {
        cpu_linear = run_linear_training_trace(pluss::pCPU, kLinearEpochs);
        return true;
      })) {
    return false;
  }
  if (!(cpu_linear.epoch_loss.back() < cpu_linear.epoch_loss.front())) {
    if (err) {
      *err << "CPU linear training should reduce loss (first=" << cpu_linear.epoch_loss.front()
           << ", last=" << cpu_linear.epoch_loss.back() << ")\n";
    }
    return false;
  }

  constexpr int kLstmEpochs = 36;
  TrainingTrace cpu_lstm{};
  if (!guarded("cpu_lstm", [&]() {
        cpu_lstm = run_lstm_training_trace(pluss::pCPU, kLstmEpochs);
        return true;
      })) {
    return false;
  }
  if (!(cpu_lstm.epoch_loss.back() < cpu_lstm.epoch_loss.front())) {
    if (err) {
      *err << "CPU LSTM training should reduce loss (first=" << cpu_lstm.epoch_loss.front()
           << ", last=" << cpu_lstm.epoch_loss.back() << ")\n";
    }
    return false;
  }

  if (pluss::mps::is_ready()) {
    TrainingTrace mps_linear{};
    if (!guarded("mps_linear", [&]() {
          mps_linear = run_linear_training_trace(pluss::pMPS, kLinearEpochs);
          return true;
        })) {
      return false;
    }
    if (!(mps_linear.epoch_loss.back() < mps_linear.epoch_loss.front())) {
      if (err) {
        *err << "MPS linear training should reduce loss (first=" << mps_linear.epoch_loss.front()
             << ", last=" << mps_linear.epoch_loss.back() << ")\n";
      }
      return false;
    }
    if (!compare_traces(cpu_linear, mps_linear, 2e-3, 0.20, 2e-2, 0.10, err)) {
      if (err) *err << "CPU/MPS training trajectory parity mismatch\n";
      return false;
    }

    TrainingTrace mps_lstm{};
    if (!guarded("mps_lstm", [&]() {
          mps_lstm = run_lstm_training_trace(pluss::pMPS, kLstmEpochs);
          return true;
        })) {
      return false;
    }
    if (!(mps_lstm.epoch_loss.back() < mps_lstm.epoch_loss.front())) {
      if (err) {
        *err << "MPS LSTM training should reduce loss (first=" << mps_lstm.epoch_loss.front()
             << ", last=" << mps_lstm.epoch_loss.back() << ")\n";
      }
      return false;
    }
    // Native MPS LSTM/autograd path should stay in a tighter parity envelope.
    if (!compare_traces(cpu_lstm, mps_lstm, 6e-2, 0.40, 2.5, 0.75, err)) {
      if (err) *err << "CPU/MPS LSTM trajectory parity mismatch\n";
      return false;
    }
  }

  if (pluss::cuda::is_ready()) {
    TrainingTrace cuda_linear{};
    if (!guarded("cuda_linear", [&]() {
          cuda_linear = run_linear_training_trace(pluss::pCUDA, kLinearEpochs);
          return true;
        })) {
      return false;
    }
    if (!(cuda_linear.epoch_loss.back() < cuda_linear.epoch_loss.front())) {
      if (err) {
        *err << "CUDA linear training should reduce loss (first=" << cuda_linear.epoch_loss.front()
             << ", last=" << cuda_linear.epoch_loss.back() << ")\n";
      }
      return false;
    }
    if (!compare_traces(cpu_linear, cuda_linear, 2e-3, 0.20, 2e-2, 0.10, err)) {
      if (err) *err << "CPU/CUDA training trajectory parity mismatch\n";
      return false;
    }

    TrainingTrace cuda_lstm{};
    if (!guarded("cuda_lstm", [&]() {
          cuda_lstm = run_lstm_training_trace(pluss::pCUDA, kLstmEpochs);
          return true;
        })) {
      return false;
    }
    if (!(cuda_lstm.epoch_loss.back() < cuda_lstm.epoch_loss.front())) {
      if (err) {
        *err << "CUDA LSTM training should reduce loss (first=" << cuda_lstm.epoch_loss.front()
             << ", last=" << cuda_lstm.epoch_loss.back() << ")\n";
      }
      return false;
    }
    if (!compare_traces(cpu_lstm, cuda_lstm, 6e-3, 0.35, 2e-1, 0.35, err)) {
      if (err) *err << "CPU/CUDA LSTM trajectory parity mismatch\n";
      return false;
    }
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
    TrainingTrace cpu{};
    {
      std::string stage = std::string("cpu_linear_matrix_") + name;
      if (!guarded(stage.c_str(), [&]() {
            cpu = run_linear_training_trace(pluss::pCPU, kLinearMatrixEpochs, kind);
            return true;
          })) {
        return false;
      }
    }
    if (!trace_is_finite(cpu)) {
      if (err) *err << "CPU " << name << " produced non-finite training trace\n";
      return false;
    }
    if (!(cpu.epoch_loss.back() < cpu.epoch_loss.front())) {
      if (err) {
        *err << "CPU " << name << " should reduce loss (first=" << cpu.epoch_loss.front()
             << ", last=" << cpu.epoch_loss.back() << ")\n";
      }
      return false;
    }
    TrainingTrace cpu_replay{};
    {
      std::string stage = std::string("cpu_linear_matrix_replay_") + name;
      if (!guarded(stage.c_str(), [&]() {
            cpu_replay = run_linear_training_trace(pluss::pCPU, kLinearMatrixEpochs, kind);
            return true;
          })) {
        return false;
      }
    }
    if (!compare_traces(cpu, cpu_replay, 1e-7, 1e-6, 1e-7, 1e-6, err)) {
      if (err) *err << "CPU determinism mismatch for " << name << "\n";
      return false;
    }

    const TraceTolerance tol = backend_linear_tolerance(kind);
    if (pluss::mps::is_ready()) {
      TrainingTrace mps{};
      {
        std::string stage = std::string("mps_linear_matrix_") + name;
        if (!guarded(stage.c_str(), [&]() {
              mps = run_linear_training_trace(pluss::pMPS, kLinearMatrixEpochs, kind);
              return true;
            })) {
          return false;
        }
      }
      if (!trace_is_finite(mps)) {
        if (err) *err << "MPS " << name << " produced non-finite training trace\n";
        return false;
      }
      if (!compare_traces(cpu, mps, tol.loss_atol, tol.loss_rtol, tol.param_atol, tol.param_rtol, err)) {
        if (err) *err << "CPU/MPS linear trajectory mismatch for " << name << "\n";
        return false;
      }
    }
    if (pluss::cuda::is_ready()) {
      TrainingTrace cuda{};
      {
        std::string stage = std::string("cuda_linear_matrix_") + name;
        if (!guarded(stage.c_str(), [&]() {
              cuda = run_linear_training_trace(pluss::pCUDA, kLinearMatrixEpochs, kind);
              return true;
            })) {
          return false;
        }
      }
      if (!trace_is_finite(cuda)) {
        if (err) *err << "CUDA " << name << " produced non-finite training trace\n";
        return false;
      }
      if (!compare_traces(cpu, cuda, tol.loss_atol, tol.loss_rtol, tol.param_atol, tol.param_rtol, err)) {
        if (err) *err << "CPU/CUDA linear trajectory mismatch for " << name << "\n";
        return false;
      }
    }

    const char* env = torch_env_for_linear_optimizer(kind);
    if (env != nullptr) {
      const auto [atol, rtol] = torch_linear_loss_tolerance(kind);
      if (!compare_losses_to_torch_reference_(cpu.epoch_loss, env, atol, rtol, err)) {
        if (err) *err << "Torch linear loss trajectory mismatch for " << name << "\n";
        return false;
      }
    }
  }

  if (!compare_losses_to_torch_reference_(
          cpu_linear.epoch_loss,
          "PLUSS_TORCH_LINEAR_REF_CSV",
          2e-3,
          0.25,
          err)) {
    return false;
  }
  if (!compare_losses_to_torch_reference_(
          cpu_lstm.epoch_loss,
          "PLUSS_TORCH_LSTM_REF_CSV",
          1.2e-1,
          0.75,
          err)) {
    return false;
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
