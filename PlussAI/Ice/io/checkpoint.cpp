#include "pluss/ai/ice/io.h"

#include "pluss/ai/ice/callback.h"
#include "pluss/ai/ice/model.h"
#include "pluss/tils.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace pluss::ai::ice::io {

namespace {

using StringMap = std::unordered_map<std::string, std::string>;
namespace optim = pluss::nn::optim;

constexpr std::string_view kMagicLine = "pluss_ice_checkpoint";
constexpr std::int64_t kCurrentSchema = 1;

[[nodiscard]] std::uint64_t fnv1a64_(std::string_view data) noexcept {
  constexpr std::uint64_t kOffsetBasis = 14695981039346656037ull;
  constexpr std::uint64_t kPrime = 1099511628211ull;
  std::uint64_t hash = kOffsetBasis;
  for (const unsigned char byte : data) {
    hash ^= byte;
    hash *= kPrime;
  }
  return hash;
}

[[nodiscard]] std::string to_hex_(std::uint64_t value) {
  std::ostringstream oss;
  oss << std::hex << std::setw(16) << std::setfill('0') << value;
  return oss.str();
}

[[nodiscard]] std::uint64_t parse_hex_u64_(std::string_view value) {
  return static_cast<std::uint64_t>(std::stoull(std::string(value), nullptr, 16));
}

void write_kv_(std::ostringstream& out, std::string_view key, std::string_view value) {
  out << key << '=';
  out << std::quoted(std::string(value)) << '\n';
}

template <class T>
void write_num_(std::ostringstream& out, std::string_view key, T value) {
  out << key << '=' << value << '\n';
}

void write_bool_(std::ostringstream& out, std::string_view key, bool value) {
  out << key << '=' << (value ? 1 : 0) << '\n';
}

[[nodiscard]] StringMap parse_kv_body_(std::string_view body) {
  StringMap out{};
  std::istringstream stream{std::string(body)};
  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty()) continue;
    const auto eq = line.find('=');
    PLUSS_CHECK(eq != std::string::npos,
                "pluss::ai::ice::io::load_checkpoint: malformed checkpoint line '" + line + "'.");
    const std::string key = line.substr(0, eq);
    const std::string value_text = line.substr(eq + 1);
    std::istringstream value_stream(value_text);
    std::string value;
    if (!value_text.empty() && value_text.front() == '"') {
      value_stream >> std::quoted(value);
    } else {
      value = value_text;
    }
    out.emplace(key, std::move(value));
  }
  return out;
}

[[nodiscard]] std::string require_(const StringMap& kv, std::string_view key) {
  const auto it = kv.find(std::string(key));
  PLUSS_CHECK(it != kv.end(),
              "pluss::ai::ice::io::load_checkpoint: missing field '" + std::string(key) + "'.");
  return it->second;
}

[[nodiscard]] std::string maybe_(const StringMap& kv, std::string_view key, std::string fallback = {}) {
  const auto it = kv.find(std::string(key));
  return it == kv.end() ? std::move(fallback) : it->second;
}

[[nodiscard]] bool maybe_bool_(const StringMap& kv, std::string_view key, bool fallback = false) {
  const auto it = kv.find(std::string(key));
  if (it == kv.end()) return fallback;
  return it->second == "1" || it->second == "true";
}

[[nodiscard]] std::int64_t maybe_i64_(const StringMap& kv, std::string_view key, std::int64_t fallback = 0) {
  const auto it = kv.find(std::string(key));
  return it == kv.end() ? fallback : std::stoll(it->second);
}

[[nodiscard]] double maybe_f64_(const StringMap& kv, std::string_view key, double fallback = 0.0) {
  const auto it = kv.find(std::string(key));
  return it == kv.end() ? fallback : std::stod(it->second);
}

[[nodiscard]] std::vector<double> parse_doubles_(std::string_view line) {
  std::vector<double> out{};
  std::istringstream iss{std::string(line)};
  double value = 0.0;
  while (iss >> value) out.push_back(value);
  return out;
}

[[nodiscard]] std::string join_doubles_(const std::vector<double>& values) {
  std::ostringstream oss{};
  oss << std::setprecision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

[[nodiscard]] std::string join_i64_(const std::vector<std::int64_t>& values) {
  std::ostringstream oss{};
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

[[nodiscard]] std::vector<std::int64_t> parse_i64s_(std::string_view line) {
  std::vector<std::int64_t> out{};
  std::istringstream iss{std::string(line)};
  std::int64_t value = 0;
  while (iss >> value) out.push_back(value);
  return out;
}

void write_series_(std::ostringstream& out, std::string_view prefix, const Series& series) {
  write_bool_(out, std::string(prefix) + ".defined", series.defined());
  if (!series.defined()) return;
  const auto shape_view = series.sizes();
  const std::vector<std::int64_t> shape(shape_view.begin(), shape_view.end());
  const auto flat = series.reshape({series.numel()}).to_vec<double>();
  write_num_(out, std::string(prefix) + ".dtype", static_cast<int>(series.scalar_type()));
  write_num_(out, std::string(prefix) + ".platform", static_cast<int>(series.platform()));
  write_kv_(out, std::string(prefix) + ".shape", join_i64_(shape));
  write_kv_(out, std::string(prefix) + ".values", join_doubles_(flat));
}

[[nodiscard]] Series read_series_(const StringMap& kv, std::string_view prefix) {
  if (!maybe_bool_(kv, std::string(prefix) + ".defined", true)) {
    return pluss::series::undefined();
  }
  const auto dtype = static_cast<ScalarType>(std::stoi(require_(kv, std::string(prefix) + ".dtype")));
  const auto platform = static_cast<Platform>(std::stoi(require_(kv, std::string(prefix) + ".platform")));
  const auto shape = parse_i64s_(require_(kv, std::string(prefix) + ".shape"));
  const auto values = parse_doubles_(require_(kv, std::string(prefix) + ".values"));
  const Config cfg = Config{}.with_dtype(dtype).with_platform(platform);
  return pluss::make_series(values, shape, cfg).to(cfg.platform, cfg.dtype);
}

void write_series_vec_(std::ostringstream& out, std::string_view prefix, const std::vector<Series>& values) {
  write_num_(out, std::string(prefix) + ".count", static_cast<std::int64_t>(values.size()));
  for (std::size_t i = 0; i < values.size(); ++i) {
    write_series_(out, std::string(prefix) + "." + std::to_string(i), values[i]);
  }
}

[[nodiscard]] std::vector<Series> read_series_vec_(const StringMap& kv, std::string_view prefix) {
  const auto count = maybe_i64_(kv, std::string(prefix) + ".count", 0);
  std::vector<Series> out{};
  out.reserve(static_cast<std::size_t>(count));
  for (std::int64_t i = 0; i < count; ++i) {
    out.push_back(read_series_(kv, std::string(prefix) + "." + std::to_string(i)));
  }
  return out;
}

void write_clip_(std::ostringstream& out, std::string_view prefix, const optim::GradientClipOptions& clip) {
  write_num_(out, std::string(prefix) + ".mode", static_cast<int>(clip.mode));
  write_num_(out, std::string(prefix) + ".threshold", clip.threshold);
  write_num_(out, std::string(prefix) + ".epsilon", clip.epsilon);
}

[[nodiscard]] optim::GradientClipOptions read_clip_(const StringMap& kv, std::string_view prefix) {
  return optim::GradientClipOptions{
      .mode = static_cast<optim::GradientClipMode>(maybe_i64_(kv, std::string(prefix) + ".mode", 0)),
      .threshold = maybe_f64_(kv, std::string(prefix) + ".threshold", 0.0),
      .epsilon = maybe_f64_(kv, std::string(prefix) + ".epsilon", 1e-12),
  };
}

void write_exec_(std::ostringstream& out, std::string_view prefix, const optim::ExecutionOptions& exec) {
  write_num_(out, std::string(prefix) + ".lane", static_cast<int>(exec.lane));
  write_bool_(out, std::string(prefix) + ".strict", exec.strict);
  write_bool_(out, std::string(prefix) + ".capturable", exec.capturable);
}

[[nodiscard]] optim::ExecutionOptions read_exec_(const StringMap& kv, std::string_view prefix) {
  return optim::ExecutionOptions{
      .lane = static_cast<optim::UpdateLane>(maybe_i64_(kv, std::string(prefix) + ".lane", 0)),
      .strict = maybe_bool_(kv, std::string(prefix) + ".strict", false),
      .capturable = maybe_bool_(kv, std::string(prefix) + ".capturable", false),
  };
}

void write_recipe_(std::ostringstream& out, const TrainOptions& recipe) {
  write_num_(out, "recipe.batch_size", recipe.batch_size);
  write_num_(out, "recipe.learning_rate", recipe.learning_rate);
  write_num_(out, "recipe.optimizer", static_cast<int>(recipe.optimizer));
  write_num_(out, "recipe.loss", static_cast<int>(recipe.loss));
  write_bool_(out, "recipe.shuffle", recipe.shuffle);
  write_num_(out, "recipe.seed", recipe.seed);
  write_bool_(out, "recipe.standardize", recipe.standardize);

  write_bool_(out, "recipe.progress.verbose", recipe.progress.verbose);
  write_num_(out, "recipe.progress.log_every_n_epochs", recipe.progress.log_every_n_epochs);
  write_bool_(out, "recipe.progress.progress_bar", recipe.progress.progress_bar);
  write_num_(out, "recipe.progress.output", static_cast<int>(recipe.progress.output));
  write_num_(out, "recipe.progress.bar_width", recipe.progress.bar_width);
  write_num_(out, "recipe.progress.precision", recipe.progress.precision);
  write_num_(out, "recipe.progress.metrics.count", recipe.progress.metrics.size());
  for (std::size_t i = 0; i < recipe.progress.metrics.size(); ++i) {
    write_kv_(out, "recipe.progress.metrics." + std::to_string(i), recipe.progress.metrics[i]);
  }

  write_num_(out, "recipe.validation.split_ratio", recipe.validation.split_ratio);
  write_num_(out, "recipe.validation.every_n_epochs", recipe.validation.every_n_epochs);
  write_num_(out, "recipe.validation.start_from_epoch", recipe.validation.start_from_epoch);
  write_kv_(out, "recipe.validation.monitor", recipe.validation.monitor);
  write_num_(out, "recipe.validation.direction", static_cast<int>(recipe.validation.direction));

  write_num_(out, "recipe.architecture_regularization.dropout", recipe.architecture_regularization.dropout);

  write_num_(out, "recipe.objective_regularization.weight_decay", recipe.objective_regularization.weight_decay);
  write_bool_(out, "recipe.objective_regularization.decoupled_weight_decay",
              recipe.objective_regularization.decoupled_weight_decay);
  write_num_(out, "recipe.objective_regularization.l1_penalty", recipe.objective_regularization.l1_penalty);
  write_num_(out, "recipe.objective_regularization.l2_penalty", recipe.objective_regularization.l2_penalty);
  write_num_(out, "recipe.objective_regularization.label_smoothing",
             recipe.objective_regularization.label_smoothing);

  write_bool_(out, "recipe.loop_regularization.early_stopping", recipe.loop_regularization.early_stopping);
  write_num_(out, "recipe.loop_regularization.patience", recipe.loop_regularization.patience);
  write_num_(out, "recipe.loop_regularization.min_delta", recipe.loop_regularization.min_delta);
  write_bool_(out, "recipe.loop_regularization.restore_best_weights",
              recipe.loop_regularization.restore_best_weights);

  write_bool_(out, "recipe.checkpoint.enabled", recipe.checkpoint.enabled);
  write_bool_(out, "recipe.checkpoint.save_last", recipe.checkpoint.save_last);
  write_bool_(out, "recipe.checkpoint.save_best", recipe.checkpoint.save_best);
  write_bool_(out, "recipe.checkpoint.save_weights_only", recipe.checkpoint.save_weights_only);
  write_bool_(out, "recipe.checkpoint.save_on_exception", recipe.checkpoint.save_on_exception);
  write_num_(out, "recipe.checkpoint.every_n_epochs", recipe.checkpoint.every_n_epochs);
  write_kv_(out, "recipe.checkpoint.dirpath", recipe.checkpoint.dirpath);
  write_kv_(out, "recipe.checkpoint.filename", recipe.checkpoint.filename);
  write_kv_(out, "recipe.checkpoint.resume_from", recipe.checkpoint.resume_from);

  write_clip_(out, "recipe.execution.grad_clip", recipe.execution.grad_clip);
  write_num_(out, "recipe.execution.max_epochs", recipe.execution.max_epochs);
  write_num_(out, "recipe.execution.accumulation_steps", recipe.execution.accumulation_steps);
  write_num_(out, "recipe.execution.fast_dev_batches", recipe.execution.fast_dev_batches);
  write_num_(out, "recipe.execution.max_train_batches", recipe.execution.max_train_batches);
  write_num_(out, "recipe.execution.max_valid_batches", recipe.execution.max_valid_batches);
  write_bool_(out, "recipe.execution.fail_on_non_finite", recipe.execution.fail_on_non_finite);
  write_bool_(out, "recipe.execution.deterministic", recipe.execution.deterministic);
  write_num_(out, "recipe.execution.precision", static_cast<int>(recipe.execution.precision));
  write_num_(out, "recipe.execution.platform", static_cast<int>(recipe.execution.platform));
}

[[nodiscard]] TrainOptions read_recipe_(const StringMap& kv) {
  TrainOptions out{};
  out.batch_size = maybe_i64_(kv, "recipe.batch_size", out.batch_size);
  out.learning_rate = maybe_f64_(kv, "recipe.learning_rate", out.learning_rate);
  out.optimizer = static_cast<OptimizerKind>(maybe_i64_(kv, "recipe.optimizer", static_cast<int>(out.optimizer)));
  out.loss = static_cast<LossKind>(maybe_i64_(kv, "recipe.loss", static_cast<int>(out.loss)));
  out.shuffle = maybe_bool_(kv, "recipe.shuffle", out.shuffle);
  out.seed = static_cast<std::uint64_t>(maybe_i64_(kv, "recipe.seed", static_cast<std::int64_t>(out.seed)));
  out.standardize = maybe_bool_(kv, "recipe.standardize", out.standardize);

  out.progress.verbose = maybe_bool_(kv, "recipe.progress.verbose", out.progress.verbose);
  out.progress.log_every_n_epochs =
      maybe_i64_(kv, "recipe.progress.log_every_n_epochs", out.progress.log_every_n_epochs);
  out.progress.progress_bar =
      maybe_bool_(kv, "recipe.progress.progress_bar", out.progress.progress_bar);
  out.progress.output = static_cast<ProgressOutput>(
      maybe_i64_(kv, "recipe.progress.output", static_cast<int>(out.progress.output)));
  out.progress.bar_width = maybe_i64_(kv, "recipe.progress.bar_width", out.progress.bar_width);
  out.progress.precision = maybe_i64_(kv, "recipe.progress.precision", out.progress.precision);
  if (const auto count = maybe_i64_(kv, "recipe.progress.metrics.count", -1); count >= 0) {
    out.progress.metrics.clear();
    out.progress.metrics.reserve(static_cast<std::size_t>(count));
    for (std::int64_t i = 0; i < count; ++i) {
      out.progress.metrics.push_back(
          maybe_(kv, "recipe.progress.metrics." + std::to_string(i), {}));
    }
  }

  out.validation.split_ratio = maybe_f64_(kv, "recipe.validation.split_ratio", out.validation.split_ratio);
  out.validation.every_n_epochs =
      maybe_i64_(kv, "recipe.validation.every_n_epochs", out.validation.every_n_epochs);
  out.validation.start_from_epoch =
      maybe_i64_(kv, "recipe.validation.start_from_epoch", out.validation.start_from_epoch);
  out.validation.monitor = maybe_(kv, "recipe.validation.monitor", out.validation.monitor);
  out.validation.direction = static_cast<MonitorDirection>(
      maybe_i64_(kv, "recipe.validation.direction", static_cast<int>(out.validation.direction)));

  out.architecture_regularization.dropout =
      maybe_f64_(kv, "recipe.architecture_regularization.dropout", out.architecture_regularization.dropout);

  out.objective_regularization.weight_decay =
      maybe_f64_(kv, "recipe.objective_regularization.weight_decay", out.objective_regularization.weight_decay);
  out.objective_regularization.decoupled_weight_decay =
      maybe_bool_(kv, "recipe.objective_regularization.decoupled_weight_decay",
                  out.objective_regularization.decoupled_weight_decay);
  out.objective_regularization.l1_penalty =
      maybe_f64_(kv, "recipe.objective_regularization.l1_penalty", out.objective_regularization.l1_penalty);
  out.objective_regularization.l2_penalty =
      maybe_f64_(kv, "recipe.objective_regularization.l2_penalty", out.objective_regularization.l2_penalty);
  out.objective_regularization.label_smoothing =
      maybe_f64_(kv, "recipe.objective_regularization.label_smoothing",
                 out.objective_regularization.label_smoothing);

  out.loop_regularization.early_stopping =
      maybe_bool_(kv, "recipe.loop_regularization.early_stopping", out.loop_regularization.early_stopping);
  out.loop_regularization.patience =
      maybe_i64_(kv, "recipe.loop_regularization.patience", out.loop_regularization.patience);
  out.loop_regularization.min_delta =
      maybe_f64_(kv, "recipe.loop_regularization.min_delta", out.loop_regularization.min_delta);
  out.loop_regularization.restore_best_weights =
      maybe_bool_(kv, "recipe.loop_regularization.restore_best_weights",
                  out.loop_regularization.restore_best_weights);

  out.checkpoint.enabled = maybe_bool_(kv, "recipe.checkpoint.enabled", out.checkpoint.enabled);
  out.checkpoint.save_last = maybe_bool_(kv, "recipe.checkpoint.save_last", out.checkpoint.save_last);
  out.checkpoint.save_best = maybe_bool_(kv, "recipe.checkpoint.save_best", out.checkpoint.save_best);
  out.checkpoint.save_weights_only =
      maybe_bool_(kv, "recipe.checkpoint.save_weights_only", out.checkpoint.save_weights_only);
  out.checkpoint.save_on_exception =
      maybe_bool_(kv, "recipe.checkpoint.save_on_exception", out.checkpoint.save_on_exception);
  out.checkpoint.every_n_epochs =
      maybe_i64_(kv, "recipe.checkpoint.every_n_epochs", out.checkpoint.every_n_epochs);
  out.checkpoint.dirpath = maybe_(kv, "recipe.checkpoint.dirpath", out.checkpoint.dirpath);
  out.checkpoint.filename = maybe_(kv, "recipe.checkpoint.filename", out.checkpoint.filename);
  out.checkpoint.resume_from = maybe_(kv, "recipe.checkpoint.resume_from", out.checkpoint.resume_from);

  out.execution.grad_clip = read_clip_(kv, "recipe.execution.grad_clip");
  out.execution.max_epochs = maybe_i64_(kv, "recipe.execution.max_epochs", out.execution.max_epochs);
  out.execution.accumulation_steps =
      maybe_i64_(kv, "recipe.execution.accumulation_steps", out.execution.accumulation_steps);
  out.execution.fast_dev_batches =
      maybe_i64_(kv, "recipe.execution.fast_dev_batches", out.execution.fast_dev_batches);
  out.execution.max_train_batches =
      maybe_i64_(kv, "recipe.execution.max_train_batches", out.execution.max_train_batches);
  out.execution.max_valid_batches =
      maybe_i64_(kv, "recipe.execution.max_valid_batches", out.execution.max_valid_batches);
  out.execution.fail_on_non_finite =
      maybe_bool_(kv, "recipe.execution.fail_on_non_finite", out.execution.fail_on_non_finite);
  out.execution.deterministic =
      maybe_bool_(kv, "recipe.execution.deterministic", out.execution.deterministic);
  out.execution.precision = static_cast<ScalarType>(
      maybe_i64_(kv, "recipe.execution.precision", static_cast<int>(out.execution.precision)));
  out.execution.platform = static_cast<TrainPlatform>(
      maybe_i64_(kv, "recipe.execution.platform", static_cast<int>(out.execution.platform)));
  return out;
}

template <class Options>
void write_common_optimizer_options_(std::ostringstream& out, std::string_view prefix, const Options& opt) {
  write_num_(out, std::string(prefix) + ".learning_rate", opt.learning_rate);
  write_num_(out, std::string(prefix) + ".weight_decay", opt.weight_decay);
  write_bool_(out, std::string(prefix) + ".maximize", opt.maximize);
  write_clip_(out, std::string(prefix) + ".clip", opt.clip);
  write_exec_(out, std::string(prefix) + ".execution", opt.execution);
}

template <class Options>
void read_common_optimizer_options_(Options& opt, const StringMap& kv, std::string_view prefix) {
  opt.learning_rate = maybe_f64_(kv, std::string(prefix) + ".learning_rate", opt.learning_rate);
  opt.weight_decay = maybe_f64_(kv, std::string(prefix) + ".weight_decay", opt.weight_decay);
  opt.maximize = maybe_bool_(kv, std::string(prefix) + ".maximize", opt.maximize);
  opt.clip = read_clip_(kv, std::string(prefix) + ".clip");
  opt.execution = read_exec_(kv, std::string(prefix) + ".execution");
}

void write_adam_options_(std::ostringstream& out, std::string_view prefix, const optim::AdamOptions& opt) {
  write_common_optimizer_options_(out, prefix, opt);
  write_num_(out, std::string(prefix) + ".beta1", opt.beta1);
  write_num_(out, std::string(prefix) + ".beta2", opt.beta2);
  write_num_(out, std::string(prefix) + ".epsilon", opt.epsilon);
  write_bool_(out, std::string(prefix) + ".amsgrad", opt.amsgrad);
}

[[nodiscard]] optim::AdamOptions read_adam_options_(const StringMap& kv, std::string_view prefix) {
  optim::AdamOptions out{};
  read_common_optimizer_options_(out, kv, prefix);
  out.beta1 = maybe_f64_(kv, std::string(prefix) + ".beta1", out.beta1);
  out.beta2 = maybe_f64_(kv, std::string(prefix) + ".beta2", out.beta2);
  out.epsilon = maybe_f64_(kv, std::string(prefix) + ".epsilon", out.epsilon);
  out.amsgrad = maybe_bool_(kv, std::string(prefix) + ".amsgrad", out.amsgrad);
  return out;
}

void write_adamw_options_(std::ostringstream& out, std::string_view prefix, const optim::AdamWOptions& opt) {
  write_common_optimizer_options_(out, prefix, opt);
  write_num_(out, std::string(prefix) + ".beta1", opt.beta1);
  write_num_(out, std::string(prefix) + ".beta2", opt.beta2);
  write_num_(out, std::string(prefix) + ".epsilon", opt.epsilon);
  write_bool_(out, std::string(prefix) + ".amsgrad", opt.amsgrad);
}

[[nodiscard]] optim::AdamWOptions read_adamw_options_(const StringMap& kv, std::string_view prefix) {
  optim::AdamWOptions out{};
  read_common_optimizer_options_(out, kv, prefix);
  out.beta1 = maybe_f64_(kv, std::string(prefix) + ".beta1", out.beta1);
  out.beta2 = maybe_f64_(kv, std::string(prefix) + ".beta2", out.beta2);
  out.epsilon = maybe_f64_(kv, std::string(prefix) + ".epsilon", out.epsilon);
  out.amsgrad = maybe_bool_(kv, std::string(prefix) + ".amsgrad", out.amsgrad);
  return out;
}

void write_sgd_options_(std::ostringstream& out, std::string_view prefix, const optim::SGDOptions& opt) {
  write_common_optimizer_options_(out, prefix, opt);
  write_num_(out, std::string(prefix) + ".momentum", opt.momentum);
  write_num_(out, std::string(prefix) + ".dampening", opt.dampening);
  write_bool_(out, std::string(prefix) + ".nesterov", opt.nesterov);
}

[[nodiscard]] optim::SGDOptions read_sgd_options_(const StringMap& kv, std::string_view prefix) {
  optim::SGDOptions out{};
  read_common_optimizer_options_(out, kv, prefix);
  out.momentum = maybe_f64_(kv, std::string(prefix) + ".momentum", out.momentum);
  out.dampening = maybe_f64_(kv, std::string(prefix) + ".dampening", out.dampening);
  out.nesterov = maybe_bool_(kv, std::string(prefix) + ".nesterov", out.nesterov);
  return out;
}

void write_rmsprop_options_(std::ostringstream& out, std::string_view prefix, const optim::RMSpropOptions& opt) {
  write_common_optimizer_options_(out, prefix, opt);
  write_num_(out, std::string(prefix) + ".alpha", opt.alpha);
  write_num_(out, std::string(prefix) + ".epsilon", opt.epsilon);
  write_num_(out, std::string(prefix) + ".momentum", opt.momentum);
  write_bool_(out, std::string(prefix) + ".centered", opt.centered);
}

[[nodiscard]] optim::RMSpropOptions read_rmsprop_options_(const StringMap& kv, std::string_view prefix) {
  optim::RMSpropOptions out{};
  read_common_optimizer_options_(out, kv, prefix);
  out.alpha = maybe_f64_(kv, std::string(prefix) + ".alpha", out.alpha);
  out.epsilon = maybe_f64_(kv, std::string(prefix) + ".epsilon", out.epsilon);
  out.momentum = maybe_f64_(kv, std::string(prefix) + ".momentum", out.momentum);
  out.centered = maybe_bool_(kv, std::string(prefix) + ".centered", out.centered);
  return out;
}

void write_adagrad_options_(std::ostringstream& out, std::string_view prefix, const optim::AdagradOptions& opt) {
  write_common_optimizer_options_(out, prefix, opt);
  write_num_(out, std::string(prefix) + ".lr_decay", opt.lr_decay);
  write_num_(out, std::string(prefix) + ".epsilon", opt.epsilon);
  write_num_(out, std::string(prefix) + ".initial_accumulator_value", opt.initial_accumulator_value);
}

[[nodiscard]] optim::AdagradOptions read_adagrad_options_(const StringMap& kv, std::string_view prefix) {
  optim::AdagradOptions out{};
  read_common_optimizer_options_(out, kv, prefix);
  out.lr_decay = maybe_f64_(kv, std::string(prefix) + ".lr_decay", out.lr_decay);
  out.epsilon = maybe_f64_(kv, std::string(prefix) + ".epsilon", out.epsilon);
  out.initial_accumulator_value =
      maybe_f64_(kv, std::string(prefix) + ".initial_accumulator_value", out.initial_accumulator_value);
  return out;
}

void write_nadam_options_(std::ostringstream& out, std::string_view prefix, const optim::NAdamOptions& opt) {
  write_common_optimizer_options_(out, prefix, opt);
  write_num_(out, std::string(prefix) + ".beta1", opt.beta1);
  write_num_(out, std::string(prefix) + ".beta2", opt.beta2);
  write_num_(out, std::string(prefix) + ".epsilon", opt.epsilon);
  write_num_(out, std::string(prefix) + ".momentum_decay", opt.momentum_decay);
  write_bool_(out, std::string(prefix) + ".decoupled_weight_decay", opt.decoupled_weight_decay);
}

[[nodiscard]] optim::NAdamOptions read_nadam_options_(const StringMap& kv, std::string_view prefix) {
  optim::NAdamOptions out{};
  read_common_optimizer_options_(out, kv, prefix);
  out.beta1 = maybe_f64_(kv, std::string(prefix) + ".beta1", out.beta1);
  out.beta2 = maybe_f64_(kv, std::string(prefix) + ".beta2", out.beta2);
  out.epsilon = maybe_f64_(kv, std::string(prefix) + ".epsilon", out.epsilon);
  out.momentum_decay = maybe_f64_(kv, std::string(prefix) + ".momentum_decay", out.momentum_decay);
  out.decoupled_weight_decay =
      maybe_bool_(kv, std::string(prefix) + ".decoupled_weight_decay", out.decoupled_weight_decay);
  return out;
}

void write_callbacks_(std::ostringstream& out, const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks) {
  for (const auto& callback : callbacks) {
    if (!callback) continue;
    const auto key = callback->state_key();
    if (key.empty()) continue;
    const auto state = callback->save_state();
    for (const auto& [field, value] : state) {
      write_kv_(out, "callback." + key + "." + field, value);
    }
  }
}

[[nodiscard]] std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
read_callback_states_(const StringMap& kv) {
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> out{};
  for (const auto& [field, value] : kv) {
    if (field.rfind("callback.", 0) != 0) continue;
    const auto callback_start = std::string("callback.").size();
    const auto next_dot = field.find('.', callback_start);
    PLUSS_CHECK(next_dot != std::string::npos,
                "pluss::ai::ice::io::load_checkpoint: malformed callback state key '" + field + "'.");
    const std::string callback_key = field.substr(callback_start, next_dot - callback_start);
    const std::string state_key = field.substr(next_dot + 1);
    PLUSS_CHECK(!callback_key.empty() && !state_key.empty(),
                "pluss::ai::ice::io::load_checkpoint: malformed callback state key '" + field + "'.");
    out[callback_key].emplace(state_key, value);
  }
  return out;
}

void write_optimizer_(std::ostringstream& out, const optim::Optimizer& optimizer) {
  if (const auto* adam = dynamic_cast<const optim::Adam*>(&optimizer)) {
    write_kv_(out, "optimizer.kind", "Adam");
    const auto state = adam->capture_state();
    write_num_(out, "optimizer.state.step", state.step);
    write_num_(out, "optimizer.state.group_count", static_cast<std::int64_t>(state.group_options.size()));
    for (std::size_t i = 0; i < state.group_options.size(); ++i) {
      write_adam_options_(out, "optimizer.group." + std::to_string(i), state.group_options[i]);
    }
    write_series_vec_(out, "optimizer.state.beta1_power", state.beta1_power);
    write_series_vec_(out, "optimizer.state.beta2_power", state.beta2_power);
    write_series_vec_(out, "optimizer.state.first_moment", state.first_moment);
    write_series_vec_(out, "optimizer.state.second_moment", state.second_moment);
    write_series_vec_(out, "optimizer.state.max_second_hat", state.max_second_hat);
    return;
  }
  if (const auto* adamw = dynamic_cast<const optim::AdamW*>(&optimizer)) {
    write_kv_(out, "optimizer.kind", "AdamW");
    const auto state = adamw->capture_state();
    write_num_(out, "optimizer.state.step", state.step);
    write_num_(out, "optimizer.state.group_count", static_cast<std::int64_t>(state.group_options.size()));
    for (std::size_t i = 0; i < state.group_options.size(); ++i) {
      write_adamw_options_(out, "optimizer.group." + std::to_string(i), state.group_options[i]);
    }
    write_series_vec_(out, "optimizer.state.beta1_power", state.beta1_power);
    write_series_vec_(out, "optimizer.state.beta2_power", state.beta2_power);
    write_series_vec_(out, "optimizer.state.first_moment", state.first_moment);
    write_series_vec_(out, "optimizer.state.second_moment", state.second_moment);
    write_series_vec_(out, "optimizer.state.max_second_hat", state.max_second_hat);
    return;
  }
  if (const auto* sgd = dynamic_cast<const optim::SGD*>(&optimizer)) {
    write_kv_(out, "optimizer.kind", "SGD");
    const auto state = sgd->capture_state();
    write_num_(out, "optimizer.state.group_count", static_cast<std::int64_t>(state.group_options.size()));
    for (std::size_t i = 0; i < state.group_options.size(); ++i) {
      write_sgd_options_(out, "optimizer.group." + std::to_string(i), state.group_options[i]);
    }
    write_series_vec_(out, "optimizer.state.velocity", state.velocity);
    return;
  }
  if (const auto* rmsprop = dynamic_cast<const optim::RMSprop*>(&optimizer)) {
    write_kv_(out, "optimizer.kind", "RMSprop");
    const auto state = rmsprop->capture_state();
    write_num_(out, "optimizer.state.group_count", static_cast<std::int64_t>(state.group_options.size()));
    for (std::size_t i = 0; i < state.group_options.size(); ++i) {
      write_rmsprop_options_(out, "optimizer.group." + std::to_string(i), state.group_options[i]);
    }
    write_series_vec_(out, "optimizer.state.square_avg", state.square_avg);
    write_series_vec_(out, "optimizer.state.momentum_buffer", state.momentum_buffer);
    write_series_vec_(out, "optimizer.state.grad_avg", state.grad_avg);
    return;
  }
  if (const auto* adagrad = dynamic_cast<const optim::Adagrad*>(&optimizer)) {
    write_kv_(out, "optimizer.kind", "Adagrad");
    const auto state = adagrad->capture_state();
    write_num_(out, "optimizer.state.step", state.step);
    write_num_(out, "optimizer.state.group_count", static_cast<std::int64_t>(state.group_options.size()));
    for (std::size_t i = 0; i < state.group_options.size(); ++i) {
      write_adagrad_options_(out, "optimizer.group." + std::to_string(i), state.group_options[i]);
    }
    write_series_vec_(out, "optimizer.state.group_steps", state.group_steps);
    write_series_vec_(out, "optimizer.state.sum_square", state.sum_square);
    return;
  }
  if (const auto* nadam = dynamic_cast<const optim::NAdam*>(&optimizer)) {
    write_kv_(out, "optimizer.kind", "NAdam");
    const auto state = nadam->capture_state();
    write_num_(out, "optimizer.state.step", state.step);
    write_num_(out, "optimizer.state.group_count", static_cast<std::int64_t>(state.group_options.size()));
    for (std::size_t i = 0; i < state.group_options.size(); ++i) {
      write_nadam_options_(out, "optimizer.group." + std::to_string(i), state.group_options[i]);
      write_num_(out, "optimizer.group." + std::to_string(i) + ".mu_product", state.mu_products[i]);
    }
    write_series_vec_(out, "optimizer.state.mu_product_scalar", state.mu_product_scalar);
    write_series_vec_(out, "optimizer.state.beta2_power", state.beta2_power);
    write_series_vec_(out, "optimizer.state.decay_power", state.decay_power);
    write_series_vec_(out, "optimizer.state.exp_avg", state.exp_avg);
    write_series_vec_(out, "optimizer.state.exp_avg_sq", state.exp_avg_sq);
    return;
  }
  PLUSS_FAIL("pluss::ai::ice::io::save_checkpoint: unsupported optimizer type.");
}

void load_optimizer_(const StringMap& kv, optim::Optimizer& optimizer) {
  const auto kind = require_(kv, "optimizer.kind");
  if (kind == "Adam") {
    auto* adam = dynamic_cast<optim::Adam*>(&optimizer);
    PLUSS_CHECK(adam != nullptr,
                "pluss::ai::ice::io::load_checkpoint: checkpoint expects Adam but the active optimizer differs.");
    optim::Adam::State state{};
    state.step = maybe_i64_(kv, "optimizer.state.step", 0);
    const auto group_count = maybe_i64_(kv, "optimizer.state.group_count", 0);
    for (std::int64_t i = 0; i < group_count; ++i) {
      state.group_options.push_back(read_adam_options_(kv, "optimizer.group." + std::to_string(i)));
    }
    state.beta1_power = read_series_vec_(kv, "optimizer.state.beta1_power");
    state.beta2_power = read_series_vec_(kv, "optimizer.state.beta2_power");
    state.first_moment = read_series_vec_(kv, "optimizer.state.first_moment");
    state.second_moment = read_series_vec_(kv, "optimizer.state.second_moment");
    state.max_second_hat = read_series_vec_(kv, "optimizer.state.max_second_hat");
    adam->restore_state(state);
    return;
  }
  if (kind == "AdamW") {
    auto* adamw = dynamic_cast<optim::AdamW*>(&optimizer);
    PLUSS_CHECK(adamw != nullptr,
                "pluss::ai::ice::io::load_checkpoint: checkpoint expects AdamW but the active optimizer differs.");
    optim::AdamW::State state{};
    state.step = maybe_i64_(kv, "optimizer.state.step", 0);
    const auto group_count = maybe_i64_(kv, "optimizer.state.group_count", 0);
    for (std::int64_t i = 0; i < group_count; ++i) {
      state.group_options.push_back(read_adamw_options_(kv, "optimizer.group." + std::to_string(i)));
    }
    state.beta1_power = read_series_vec_(kv, "optimizer.state.beta1_power");
    state.beta2_power = read_series_vec_(kv, "optimizer.state.beta2_power");
    state.first_moment = read_series_vec_(kv, "optimizer.state.first_moment");
    state.second_moment = read_series_vec_(kv, "optimizer.state.second_moment");
    state.max_second_hat = read_series_vec_(kv, "optimizer.state.max_second_hat");
    adamw->restore_state(state);
    return;
  }
  if (kind == "SGD") {
    auto* sgd = dynamic_cast<optim::SGD*>(&optimizer);
    PLUSS_CHECK(sgd != nullptr,
                "pluss::ai::ice::io::load_checkpoint: checkpoint expects SGD but the active optimizer differs.");
    optim::SGD::State state{};
    const auto group_count = maybe_i64_(kv, "optimizer.state.group_count", 0);
    for (std::int64_t i = 0; i < group_count; ++i) {
      state.group_options.push_back(read_sgd_options_(kv, "optimizer.group." + std::to_string(i)));
    }
    state.velocity = read_series_vec_(kv, "optimizer.state.velocity");
    sgd->restore_state(state);
    return;
  }
  if (kind == "RMSprop") {
    auto* rmsprop = dynamic_cast<optim::RMSprop*>(&optimizer);
    PLUSS_CHECK(rmsprop != nullptr,
                "pluss::ai::ice::io::load_checkpoint: checkpoint expects RMSprop but the active optimizer differs.");
    optim::RMSprop::State state{};
    const auto group_count = maybe_i64_(kv, "optimizer.state.group_count", 0);
    for (std::int64_t i = 0; i < group_count; ++i) {
      state.group_options.push_back(read_rmsprop_options_(kv, "optimizer.group." + std::to_string(i)));
    }
    state.square_avg = read_series_vec_(kv, "optimizer.state.square_avg");
    state.momentum_buffer = read_series_vec_(kv, "optimizer.state.momentum_buffer");
    state.grad_avg = read_series_vec_(kv, "optimizer.state.grad_avg");
    rmsprop->restore_state(state);
    return;
  }
  if (kind == "Adagrad") {
    auto* adagrad = dynamic_cast<optim::Adagrad*>(&optimizer);
    PLUSS_CHECK(adagrad != nullptr,
                "pluss::ai::ice::io::load_checkpoint: checkpoint expects Adagrad but the active optimizer differs.");
    optim::Adagrad::State state{};
    state.step = maybe_i64_(kv, "optimizer.state.step", 0);
    const auto group_count = maybe_i64_(kv, "optimizer.state.group_count", 0);
    for (std::int64_t i = 0; i < group_count; ++i) {
      state.group_options.push_back(read_adagrad_options_(kv, "optimizer.group." + std::to_string(i)));
    }
    state.group_steps = read_series_vec_(kv, "optimizer.state.group_steps");
    state.sum_square = read_series_vec_(kv, "optimizer.state.sum_square");
    adagrad->restore_state(state);
    return;
  }
  if (kind == "NAdam") {
    auto* nadam = dynamic_cast<optim::NAdam*>(&optimizer);
    PLUSS_CHECK(nadam != nullptr,
                "pluss::ai::ice::io::load_checkpoint: checkpoint expects NAdam but the active optimizer differs.");
    optim::NAdam::State state{};
    state.step = maybe_i64_(kv, "optimizer.state.step", 0);
    const auto group_count = maybe_i64_(kv, "optimizer.state.group_count", 0);
    state.mu_products.reserve(static_cast<std::size_t>(group_count));
    for (std::int64_t i = 0; i < group_count; ++i) {
      state.group_options.push_back(read_nadam_options_(kv, "optimizer.group." + std::to_string(i)));
      state.mu_products.push_back(maybe_f64_(kv, "optimizer.group." + std::to_string(i) + ".mu_product", 1.0));
    }
    state.mu_product_scalar = read_series_vec_(kv, "optimizer.state.mu_product_scalar");
    state.beta2_power = read_series_vec_(kv, "optimizer.state.beta2_power");
    state.decay_power = read_series_vec_(kv, "optimizer.state.decay_power");
    state.exp_avg = read_series_vec_(kv, "optimizer.state.exp_avg");
    state.exp_avg_sq = read_series_vec_(kv, "optimizer.state.exp_avg_sq");
    nadam->restore_state(state);
    return;
  }
  PLUSS_FAIL("pluss::ai::ice::io::load_checkpoint: unsupported optimizer type '" + kind + "'.");
}

} // namespace

void save_checkpoint(std::string_view filepath,
                     const pluss::ai::ice::Model& model,
                     const optim::Optimizer& optimizer,
                     const CheckpointState& state,
                     const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(), "pluss::ai::ice::io::save_checkpoint: output path cannot be empty.");

  const std::string base_path(filepath);
  const std::string weights_path = base_path + ".weights";
  model.save(weights_path);

  std::ostringstream body{};
  write_recipe_(body, state.recipe);
  write_num_(body, "state.next_epoch", state.next_epoch);
  write_num_(body, "state.global_step", state.global_step);
  write_bool_(body, "state.best_metric_defined", state.best_metric.has_value());
  if (state.best_metric.has_value()) {
    write_num_(body, "state.best_metric", *state.best_metric);
  }
  write_kv_(body, "state.best_metric_name", state.best_metric_name);
  write_kv_(body, "state.best_checkpoint_path", state.best_checkpoint_path);
  write_kv_(body, "state.last_checkpoint_path", state.last_checkpoint_path);
  write_kv_(body, "state.stop_reason", state.stop_reason);
  write_kv_(body, "state.rng_state", state.rng_state);
  write_kv_(body, "state.weights_path", weights_path);
  write_callbacks_(body, callbacks);
  write_optimizer_(body, optimizer);

  const auto body_text = body.str();
  std::ofstream out(base_path);
  PLUSS_CHECK(out.good(),
              "pluss::ai::ice::io::save_checkpoint: failed to open '" + base_path + "'.");
  out << kMagicLine << '\n';
  out << "schema=" << kCurrentSchema << '\n';
  out << "checksum=" << to_hex_(fnv1a64_(body_text)) << '\n';
  out << body_text;
  PLUSS_CHECK(out.good(),
              "pluss::ai::ice::io::save_checkpoint: failed while writing '" + base_path + "'.");
}

CheckpointState load_checkpoint(
    std::string_view filepath,
    pluss::ai::ice::Model& model,
    optim::Optimizer& optimizer,
    const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks) {
  PLUSS_CALL_GUARD();
  // Deliberately not used to restore callback state here, unlike
  // save_checkpoint's own symmetric use of `callbacks` -- a callback such
  // as EarlyStopCallback unconditionally resets its own state in
  // on_run_begin, which the caller's own loop (see Coach::learn) always
  // fires *after* this function returns. Restoring here would just be
  // overwritten; the caller applies out.callback_state once its own
  // on_run_begin-equivalent step has already run. `callbacks` stays in
  // the signature for symmetry with save_checkpoint and so a future
  // caller-side convenience wrapper has it to hand without widening the
  // ABI.
  static_cast<void>(callbacks);
  PLUSS_CHECK(!filepath.empty(), "pluss::ai::ice::io::load_checkpoint: input path cannot be empty.");
  std::ifstream in{std::string(filepath)};
  PLUSS_CHECK(in.good(),
              "pluss::ai::ice::io::load_checkpoint: failed to open '" + std::string(filepath) + "'.");

  std::string line{};
  PLUSS_CHECK(std::getline(in, line) && line == kMagicLine,
              "pluss::ai::ice::io::load_checkpoint: invalid checkpoint header.");
  PLUSS_CHECK(std::getline(in, line) && line.rfind("schema=", 0) == 0,
              "pluss::ai::ice::io::load_checkpoint: missing schema header.");
  const auto schema = std::stoll(line.substr(7));
  PLUSS_CHECK(schema == kCurrentSchema,
              "pluss::ai::ice::io::load_checkpoint: unsupported schema " + std::to_string(schema) + ".");
  PLUSS_CHECK(std::getline(in, line) && line.rfind("checksum=", 0) == 0,
              "pluss::ai::ice::io::load_checkpoint: missing checksum header.");
  const auto expected_checksum = parse_hex_u64_(std::string_view(line).substr(9));

  std::ostringstream body_buffer{};
  body_buffer << in.rdbuf();
  const auto body_text = body_buffer.str();
  PLUSS_CHECK(fnv1a64_(body_text) == expected_checksum,
              "pluss::ai::ice::io::load_checkpoint: checksum mismatch, artifact is truncated or corrupted.");

  const auto kv = parse_kv_body_(body_text);
  model.load(require_(kv, "state.weights_path"));
  load_optimizer_(kv, optimizer);

  CheckpointState out{};
  out.recipe = read_recipe_(kv);
  out.next_epoch = maybe_i64_(kv, "state.next_epoch", 0);
  out.global_step = maybe_i64_(kv, "state.global_step", 0);
  if (maybe_bool_(kv, "state.best_metric_defined", false)) {
    out.best_metric = maybe_f64_(kv, "state.best_metric", 0.0);
  }
  out.best_metric_name = maybe_(kv, "state.best_metric_name");
  out.best_checkpoint_path = maybe_(kv, "state.best_checkpoint_path");
  out.last_checkpoint_path = maybe_(kv, "state.last_checkpoint_path");
  out.stop_reason = maybe_(kv, "state.stop_reason");
  out.rng_state = maybe_(kv, "state.rng_state");
  out.callback_state = read_callback_states_(kv);
  return out;
}

} // namespace pluss::ai::ice::io
