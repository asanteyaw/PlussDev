#include "pluss/ai/ice/callback.h"
#include "pluss/ai/ice/model.h"

#include "pluss/tils.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace pluss::ai::ice {

namespace {

[[nodiscard]] std::string lower_ascii_(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] MonitorDirection resolve_monitor_direction_(const ValidationPolicy& validation) {
  if (validation.direction != MonitorDirection::Auto) {
    return validation.direction;
  }
  const std::string monitor = lower_ascii_(validation.monitor);
  if (monitor.find("acc") != std::string::npos || monitor.find("auc") != std::string::npos ||
      monitor.find("f1") != std::string::npos || monitor.find("precision") != std::string::npos ||
      monitor.find("recall") != std::string::npos || monitor.find("map") != std::string::npos ||
      monitor.find("ndcg") != std::string::npos || monitor.find("score") != std::string::npos) {
    return MonitorDirection::Max;
  }
  return MonitorDirection::Min;
}

[[nodiscard]] bool monitor_improved_(double current,
                                     double best,
                                     const ValidationPolicy& validation,
                                     double min_delta) {
  switch (resolve_monitor_direction_(validation)) {
    case MonitorDirection::Min: return current < best - min_delta;
    case MonitorDirection::Max: return current > best + min_delta;
    case MonitorDirection::Auto: break;
  }
  PLUSS_FAIL("pluss::ai::ice::EarlyStopCallback: unreachable monitor direction.");
}

[[nodiscard]] std::vector<Series> snapshot_parameters_(const std::vector<Series*>& parameters) {
  std::vector<Series> out{};
  out.reserve(parameters.size());
  for (const Series* parameter : parameters) {
    PLUSS_CHECK(parameter != nullptr && parameter->defined(),
                "pluss::ai::ice::EarlyStopCallback: cannot snapshot an undefined parameter.");
    out.push_back(parameter->clone());
  }
  return out;
}

void restore_parameters_(const std::vector<Series*>& parameters, const std::vector<Series>& snapshot) {
  PLUSS_CHECK(parameters.size() == snapshot.size(),
              "pluss::ai::ice::EarlyStopCallback: parameter restore snapshot size mismatch.");
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    parameters[i]->copy_(snapshot[i]);
  }
}

template <class Fn>
void for_each_callback_(const std::vector<std::shared_ptr<Callback>>& callbacks, Fn&& fn) {
  for (const auto& callback : callbacks) {
    if (callback) fn(*callback);
  }
}

} // namespace

std::optional<double> RunState::metric(std::string_view name) const {
  const auto it = metrics.find(std::string(name));
  if (it == metrics.end()) return std::nullopt;
  return it->second;
}

void RunState::put_metric(std::string name, double value) {
  metrics[std::move(name)] = value;
}

void Callback::on_run_begin(RunState&) {}
void Callback::on_run_end(RunState&) {}
void Callback::on_epoch_begin(RunState&) {}
void Callback::on_epoch_end(RunState&) {}
void Callback::on_batch_begin(RunState&) {}
void Callback::on_batch_end(RunState&) {}
void Callback::on_before_backward(RunState&) {}
void Callback::on_after_backward(RunState&) {}
void Callback::on_before_step(RunState&) {}
void Callback::on_checkpoint_save(RunState&, std::string_view) {}
void Callback::on_checkpoint_load(RunState&, std::string_view) {}
void Callback::on_exception(RunState&) {}
std::string Callback::state_key() const { return {}; }
std::unordered_map<std::string, std::string> Callback::save_state() const { return {}; }
void Callback::load_state(const std::unordered_map<std::string, std::string>&) {}

CallbackChain::CallbackChain(std::vector<std::shared_ptr<Callback>> callbacks)
    : callbacks_(std::move(callbacks)) {}

void CallbackChain::add(std::shared_ptr<Callback> callback) {
  if (callback) callbacks_.push_back(std::move(callback));
}

void CallbackChain::on_run_begin(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_run_begin(state); });
}

void CallbackChain::on_run_end(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_run_end(state); });
}

void CallbackChain::on_epoch_begin(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_epoch_begin(state); });
}

void CallbackChain::on_epoch_end(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_epoch_end(state); });
}

void CallbackChain::on_batch_begin(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_batch_begin(state); });
}

void CallbackChain::on_batch_end(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_batch_end(state); });
}

void CallbackChain::on_before_backward(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_before_backward(state); });
}

void CallbackChain::on_after_backward(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_after_backward(state); });
}

void CallbackChain::on_before_step(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_before_step(state); });
}

void CallbackChain::on_checkpoint_save(RunState& state, std::string_view filepath) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_checkpoint_save(state, filepath); });
}

void CallbackChain::on_checkpoint_load(RunState& state, std::string_view filepath) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_checkpoint_load(state, filepath); });
}

void CallbackChain::on_exception(RunState& state) {
  for_each_callback_(callbacks_, [&](Callback& callback) { callback.on_exception(state); });
}

namespace {

void validate_progress_policy_(const ProgressPolicy& policy) {
  PLUSS_CHECK(policy.log_every_n_epochs > 0,
              "pluss::ai::ice::EpochLoggerCallback: log_every_n_epochs must be positive.");
  PLUSS_CHECK(policy.bar_width > 0,
              "pluss::ai::ice::EpochLoggerCallback: bar_width must be positive.");
  PLUSS_CHECK(policy.precision >= 0,
              "pluss::ai::ice::EpochLoggerCallback: precision must be non-negative.");
  PLUSS_CHECK(policy.output == ProgressOutput::Stdout || policy.output == ProgressOutput::Stderr,
              "pluss::ai::ice::EpochLoggerCallback: output must be Stdout or Stderr.");
  for (const auto& metric : policy.metrics) {
    PLUSS_CHECK(!metric.empty(),
                "pluss::ai::ice::EpochLoggerCallback: metric names must not be empty.");
  }
}

[[nodiscard]] std::ostream& progress_stream_(ProgressOutput output) {
  return output == ProgressOutput::Stderr ? std::cerr : std::cout;
}

} // namespace

EpochLoggerCallback::EpochLoggerCallback(ProgressPolicy policy)
    : policy_(std::move(policy)), output_(&progress_stream_(policy_.output)) {
  validate_progress_policy_(policy_);
}

EpochLoggerCallback::EpochLoggerCallback(ProgressPolicy policy, std::ostream& output)
    : policy_(std::move(policy)), output_(&output) {
  validate_progress_policy_(policy_);
}

void EpochLoggerCallback::on_epoch_end(RunState& state) {
  if (state.stage != StepStage::Learn) return;

  const std::int64_t completed = state.epoch + 1;
  const std::int64_t total = state.recipe == nullptr ? 0 : state.recipe->execution.max_epochs;
  const bool scheduled = completed % policy_.log_every_n_epochs == 0;
  const bool fast_dev_complete = state.recipe != nullptr &&
                                 state.recipe->execution.fast_dev_batches > 0 && completed >= 1;
  const bool final_epoch = (total > 0 && completed >= total) || fast_dev_complete;
  if (!scheduled && !final_epoch && !state.stop_requested) return;

  std::ostringstream line{};
  line << std::setprecision(static_cast<int>(policy_.precision));
  if (policy_.progress_bar) {
    const double fraction = total > 0
                                ? std::clamp(static_cast<double>(completed) / static_cast<double>(total), 0.0, 1.0)
                                : 0.0;
    const auto filled = static_cast<std::int64_t>(fraction * static_cast<double>(policy_.bar_width));
    line << '\r' << '[';
    for (std::int64_t i = 0; i < policy_.bar_width; ++i) {
      line << (i < filled ? '=' : ' ');
    }
    line << "] " << static_cast<int>(fraction * 100.0) << "% ";
  }

  line << "epoch " << completed;
  if (total > 0) line << '/' << total;

  bool first_metric = true;
  for (const auto& name : policy_.metrics) {
    const auto value = state.metric(name);
    if (!value.has_value()) continue;
    line << (first_metric ? ": " : ", ") << name << '=' << *value;
    first_metric = false;
  }
  if (state.stop_requested && !state.stop_reason.empty()) {
    line << (first_metric ? ": " : ", ") << "stopped=" << state.stop_reason;
  }

  const bool finish = !policy_.progress_bar || final_epoch || state.stop_requested;
  line << (finish ? '\n' : '\r');
  *output_ << line.str() << std::flush;
  line_open_ = policy_.progress_bar && !finish;
}

void EpochLoggerCallback::finish_line_() {
  if (!line_open_) return;
  *output_ << '\n' << std::flush;
  line_open_ = false;
}

void EpochLoggerCallback::on_run_end(RunState&) {
  finish_line_();
}

void EpochLoggerCallback::on_exception(RunState&) {
  finish_line_();
}

void EarlyStopCallback::on_run_begin(RunState& state) {
  static_cast<void>(state);
  // Checkpoint resume (if any) restores these fields via load_state(),
  // which Coach::learn calls after this hook -- so a resumed run's reset
  // here is always overwritten with the real values before any epoch runs;
  // nothing here needs to special-case resuming.
  best_value_ = std::nullopt;
  best_epoch_ = -1;
  stopped_epoch_ = -1;
  wait_count_ = 0;
  best_parameters_.clear();
}

void EarlyStopCallback::on_epoch_end(RunState& state) {
  if (state.stage != StepStage::Learn) return;
  if (state.epoch < validation_.start_from_epoch) return;

  // Validation runs on the coach's cadence: every `every_n_epochs` epochs
  // counted from `start_from_epoch`. Between those epochs the monitored
  // metric is absent by design, so the run neither improves nor ages.
  const bool evaluation_epoch =
      (state.epoch - validation_.start_from_epoch) % validation_.every_n_epochs == 0;
  const auto current = state.metric(validation_.monitor);
  if (!evaluation_epoch && !current.has_value()) return;
  PLUSS_CHECK(current.has_value(),
              "pluss::ai::ice::EarlyStopCallback: monitored metric '" + validation_.monitor +
                  "' was not produced for this epoch.");

  if (!best_value_.has_value() ||
      monitor_improved_(*current, *best_value_, validation_, regularization_.min_delta)) {
    best_value_ = *current;
    best_epoch_ = state.epoch;
    wait_count_ = 0;
    if (regularization_.restore_best_weights && state.parameters != nullptr) {
      best_parameters_ = snapshot_parameters_(*state.parameters);
    }
    return;
  }

  ++wait_count_;
  if (regularization_.early_stopping && wait_count_ >= regularization_.patience) {
    state.stop_requested = true;
    state.stop_reason = "early_stopping";
    stopped_epoch_ = state.epoch;
  }
}

void EarlyStopCallback::on_run_end(RunState& state) {
  if (!regularization_.restore_best_weights) return;
  if (!best_parameters_.empty()) {
    PLUSS_CHECK(state.parameters != nullptr,
                "pluss::ai::ice::EarlyStopCallback: restore_best_weights requested but no parameters were provided.");
    restore_parameters_(*state.parameters, best_parameters_);
    return;
  }
  if (state.model != nullptr && !state.best_checkpoint_path.empty()) {
    state.model->load(state.best_checkpoint_path + ".weights");
  }
}

std::string EarlyStopCallback::state_key() const {
  return "early_stop";
}

std::unordered_map<std::string, std::string> EarlyStopCallback::save_state() const {
  std::unordered_map<std::string, std::string> out{};
  out.emplace("monitor", validation_.monitor);
  out.emplace("direction", std::to_string(static_cast<int>(validation_.direction)));
  out.emplace("start_from_epoch", std::to_string(validation_.start_from_epoch));
  out.emplace("early_stopping", regularization_.early_stopping ? "1" : "0");
  out.emplace("patience", std::to_string(regularization_.patience));
  out.emplace("min_delta", std::to_string(regularization_.min_delta));
  out.emplace("restore_best_weights", regularization_.restore_best_weights ? "1" : "0");
  out.emplace("best_value_defined", best_value_.has_value() ? "1" : "0");
  if (best_value_.has_value()) out.emplace("best_value", std::to_string(*best_value_));
  out.emplace("best_epoch", std::to_string(best_epoch_));
  out.emplace("stopped_epoch", std::to_string(stopped_epoch_));
  out.emplace("wait_count", std::to_string(wait_count_));
  return out;
}

void EarlyStopCallback::load_state(const std::unordered_map<std::string, std::string>& state) {
  const auto find_or = [&](std::string_view key, std::string_view fallback) {
    const auto it = state.find(std::string(key));
    return it == state.end() ? std::string(fallback) : it->second;
  };
  validation_.monitor = find_or("monitor", validation_.monitor);
  validation_.direction =
      static_cast<MonitorDirection>(std::stoi(find_or("direction", std::to_string(static_cast<int>(validation_.direction)))));
  validation_.start_from_epoch = std::stoll(find_or("start_from_epoch", std::to_string(validation_.start_from_epoch)));
  regularization_.early_stopping = find_or("early_stopping", regularization_.early_stopping ? "1" : "0") == "1";
  regularization_.patience = std::stoll(find_or("patience", std::to_string(regularization_.patience)));
  regularization_.min_delta = std::stod(find_or("min_delta", std::to_string(regularization_.min_delta)));
  regularization_.restore_best_weights =
      find_or("restore_best_weights", regularization_.restore_best_weights ? "1" : "0") == "1";
  const bool best_defined = find_or("best_value_defined", "0") == "1";
  if (best_defined) {
    best_value_ = std::stod(find_or("best_value", "0"));
  } else {
    best_value_ = std::nullopt;
  }
  best_epoch_ = std::stoll(find_or("best_epoch", "-1"));
  stopped_epoch_ = std::stoll(find_or("stopped_epoch", "-1"));
  wait_count_ = std::stoll(find_or("wait_count", "0"));
  best_parameters_.clear();
}

} // namespace pluss::ai::ice
