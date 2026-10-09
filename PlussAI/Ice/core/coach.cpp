#include "pluss/ai/ice/coach.h"

#include "pluss/ai/ice/io.h"
#include "pluss/ai/ice/model.h"
#include "pluss/ai/ice/no_grad.h"
#include "pluss/ai/ice/optim.h"

#include "pluss/loader/loader.h"
#include "pluss/tils.h"
#include "table/DataFrame.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <random>
#include <sstream>

namespace pluss::ai::ice {

namespace {

[[nodiscard]] std::int64_t batch_rows_(const Batch& batch) {
  if (batch.has_targets() && !batch.targets().sizes().empty()) {
    return batch.targets().sizes()[0];
  }
  if (batch.has_inputs() && !batch.inputs().sizes().empty()) {
    return batch.inputs().sizes()[0];
  }
  for (const auto& field : batch.fields()) {
    if (field.values.defined() && !field.values.sizes().empty()) {
      return field.values.sizes()[0];
    }
  }
  return 1;
}

[[nodiscard]] bool is_floating_dtype_(ScalarType dtype) {
  return dtype == ScalarType::Float16 || dtype == ScalarType::Float32 || dtype == ScalarType::Float64;
}

[[nodiscard]] ScalarType resolve_precision_(ScalarType requested) {
  return is_floating_dtype_(requested) ? requested : ScalarType::Float32;
}

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
  PLUSS_FAIL("pluss::ai::ice::Coach: unreachable monitor direction.");
}

[[nodiscard]] std::string checkpoint_stem_(const CheckpointPolicy& policy) {
  return policy.filename.empty() ? std::string("ice-checkpoint") : policy.filename;
}

[[nodiscard]] std::string checkpoint_path_(const CheckpointPolicy& policy, std::string_view suffix) {
  const auto stem = checkpoint_stem_(policy);
  const std::filesystem::path base =
      policy.dirpath.empty() ? std::filesystem::current_path() : std::filesystem::path(policy.dirpath);
  return (base / (stem + std::string(suffix))).string();
}

void maybe_save_checkpoint_(const CheckpointPolicy& policy,
                            const Model& model,
                            const pluss::nn::optim::Optimizer& optimizer,
                            RunState& state,
                            CallbackChain& callbacks,
                            const io::CheckpointState& snapshot,
                            std::string_view path) {
  if (!policy.enabled) return;
  std::filesystem::create_directories(std::filesystem::path(path).parent_path());
  if (policy.save_weights_only) {
    model.save(std::string(path) + ".weights");
  } else {
    io::save_checkpoint(path, model, optimizer, snapshot, callbacks.callbacks());
  }
  // `path` may alias one of the fields assigned below (the save_last call
  // site passes state.last_checkpoint_path itself) -- copy it to a stable
  // local first so neither the field writes nor the callback see a
  // dangling view into a buffer that assignment just freed.
  const std::string path_copy(path);
  state.checkpoint_path = path_copy;
  state.last_checkpoint_path = path_copy;
  callbacks.on_checkpoint_save(state, path_copy);
}

} // namespace

Coach& Coach::add_callback(std::shared_ptr<Callback> callback) {
  if (callback) callbacks_.push_back(std::move(callback));
  return *this;
}

namespace {

// Copies each epoch's metrics out of RunState into the model's training
// report. It only reads state, so it never changes what a run does.
class TrainingRecorder final : public Callback {
public:
  explicit TrainingRecorder(TrainingReport* report) : report_(report) {}

  void on_run_begin(RunState&) override {
    report_->epochs.clear();
    report_->global_steps = 0;
    report_->stop_reason.clear();
  }

  void on_epoch_end(RunState& state) override {
    EpochReport epoch{};
    epoch.epoch = state.epoch;
    epoch.train_loss = state.metric("loss").value_or(0.0);
    epoch.val_loss = state.metric("val_loss");
    report_->epochs.push_back(epoch);
  }

  void on_run_end(RunState& state) override {
    report_->global_steps = state.global_step;
    report_->stop_reason = state.stop_reason;
  }

  void on_exception(RunState& state) override {
    report_->global_steps = state.global_step;
    report_->stop_reason = "exception";
  }

private:
  TrainingReport* report_;
};

} // namespace

CallbackChain Coach::build_train_callbacks_(const TrainOptions& options, bool has_validation) const {
  CallbackChain chain(callbacks_);
  if (has_validation &&
      (options.loop_regularization.early_stopping ||
       options.loop_regularization.restore_best_weights)) {
    chain.add(std::make_shared<EarlyStopCallback>(options.validation, options.loop_regularization));
  }
  if (options.progress.verbose) {
    chain.add(std::make_shared<EpochLoggerCallback>(options.progress));
  }
  return chain;
}

void Coach::train_loop_(Model& model, Source& source, const TrainOptions& options) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(options.execution.max_epochs > 0 && options.batch_size > 0,
              "pluss::ai::ice::Coach::learn: max_epochs and batch_size must be positive.");
  PLUSS_CHECK(options.validation.every_n_epochs > 0,
              "pluss::ai::ice::Coach::learn: validation.every_n_epochs must be positive.");
  PLUSS_CHECK(options.validation.start_from_epoch >= 0,
              "pluss::ai::ice::Coach::learn: validation.start_from_epoch must be non-negative.");
  PLUSS_CHECK(options.execution.accumulation_steps > 0,
              "pluss::ai::ice::Coach::learn: accumulation_steps must be positive.");
  PLUSS_CHECK(options.execution.fast_dev_batches >= 0,
              "pluss::ai::ice::Coach::learn: fast_dev_batches must be non-negative.");
  PLUSS_CHECK(options.execution.max_train_batches >= 0,
              "pluss::ai::ice::Coach::learn: max_train_batches must be non-negative.");
  PLUSS_CHECK(options.execution.max_valid_batches >= 0,
              "pluss::ai::ice::Coach::learn: max_valid_batches must be non-negative.");
  PLUSS_CHECK(options.checkpoint.every_n_epochs > 0,
              "pluss::ai::ice::Coach::learn: checkpoint.every_n_epochs must be positive.");
  PLUSS_CHECK(options.checkpoint.resume_from.empty() || options.checkpoint.enabled,
              "pluss::ai::ice::Coach::learn: checkpoint.resume_from requires checkpoint.enabled=true.");
  PLUSS_CHECK(options.checkpoint.resume_from.empty() || !options.checkpoint.save_weights_only,
              "pluss::ai::ice::Coach::learn: cannot resume training from a weights-only checkpoint.");
  PLUSS_CHECK(options.checkpoint.resume_from.empty() ||
                  !options.loop_regularization.restore_best_weights ||
                  (options.checkpoint.save_best && !options.checkpoint.save_weights_only),
              "pluss::ai::ice::Coach::learn: resume with restore_best_weights requires save_best=true "
              "and save_weights_only=false.");

  const auto parameters = model.all_parameters_();
  PLUSS_CHECK(!parameters.empty(),
              "pluss::ai::ice::Coach::learn: model has no registered parameters "
              "(call register_parameter/register_module in the constructor).");
  source.prepare();
  const auto& learn_batches = source.learn_batches();
  const auto& valid_batches = source.valid_batches();
  PLUSS_CHECK(!learn_batches.empty(),
              "pluss::ai::ice::Coach::learn: source produced no learn batches.");

  const Platform target_platform = resolve_platform(options.execution.platform);
  const ScalarType target_dtype = resolve_precision_(options.execution.precision);
  model.to(target_platform, target_dtype);
  if (options.execution.deterministic) {
    pluss::tils::rng_seed(options.seed);
  }

  auto optimizer = make_optimizer(parameters, options);
  const bool has_val = !valid_batches.empty();
  CallbackChain callbacks = build_train_callbacks_(options, has_val);
  callbacks.add(std::make_shared<TrainingRecorder>(&model.training_report_));
  std::mt19937_64 rng(options.seed);
  std::int64_t start_epoch = 0;
  std::optional<io::CheckpointState> loaded_checkpoint = std::nullopt;
  if (!options.checkpoint.resume_from.empty()) {
    loaded_checkpoint = io::load_checkpoint(options.checkpoint.resume_from, model, *optimizer, callbacks.callbacks());
    PLUSS_CHECK(loaded_checkpoint->next_epoch >= 0,
                "pluss::ai::ice::Coach::learn: checkpoint next_epoch must be non-negative.");
    start_epoch = loaded_checkpoint->next_epoch;
    if (!loaded_checkpoint->rng_state.empty()) {
      std::istringstream rng_stream(loaded_checkpoint->rng_state);
      rng_stream >> rng;
      PLUSS_CHECK(!rng_stream.fail(),
                  "pluss::ai::ice::Coach::learn: failed to restore RNG state from checkpoint.");
    }
  }
  RunState state{
      .model = &model,
      .stage = StepStage::Learn,
      .target_platform = target_platform,
      .epoch = start_epoch - 1,
      .batch_index = -1,
      .global_step = 0,
      .rows = 0,
      .metrics = {},
      .parameters = &parameters,
      .recipe = &options,
      .stop_requested = false,
      .stop_reason = {},
      .exception_message = {},
      .checkpoint_path = {},
      .best_checkpoint_path = {},
      .last_checkpoint_path = {},
      .best_metric = std::nullopt,
      .best_metric_name = options.validation.monitor,
  };

  const std::int64_t accumulation_steps = options.execution.accumulation_steps;
  const std::int64_t max_train_batches =
      options.execution.fast_dev_batches > 0 ? options.execution.fast_dev_batches : options.execution.max_train_batches;
  const std::int64_t max_valid_batches =
      options.execution.fast_dev_batches > 0 ? options.execution.fast_dev_batches : options.execution.max_valid_batches;

  try {
    model.train(true);
    callbacks.on_run_begin(state);
    if (loaded_checkpoint.has_value()) {
      // Restore each callback's own state now, *after* on_run_begin --
      // on_run_begin unconditionally resets a stateful callback like
      // EarlyStopCallback, so restoring beforehand would just be
      // overwritten. load_checkpoint itself cannot do this restoration
      // (it runs before on_run_begin), so it stays the caller's job here.
      for (const auto& callback : callbacks.callbacks()) {
        if (!callback) continue;
        const auto key = callback->state_key();
        if (key.empty()) continue;
        const auto it = loaded_checkpoint->callback_state.find(key);
        if (it != loaded_checkpoint->callback_state.end()) {
          callback->load_state(it->second);
        }
      }
      state.epoch = start_epoch - 1;
      state.global_step = loaded_checkpoint->global_step;
      state.best_metric = loaded_checkpoint->best_metric;
      state.best_metric_name =
          loaded_checkpoint->best_metric_name.empty() ? options.validation.monitor : loaded_checkpoint->best_metric_name;
      state.best_checkpoint_path = loaded_checkpoint->best_checkpoint_path;
      state.last_checkpoint_path = loaded_checkpoint->last_checkpoint_path;
      state.checkpoint_path = options.checkpoint.resume_from;
      callbacks.on_checkpoint_load(state, options.checkpoint.resume_from);
    }
    for (std::int64_t epoch = start_epoch; epoch < options.execution.max_epochs; ++epoch) {
      state.stage = StepStage::Learn;
      state.epoch = epoch;
      state.batch_index = -1;
      state.rows = 0;
      state.metrics.clear();
      callbacks.on_epoch_begin(state);
      if (state.stop_requested) break;

      double train_weighted_loss_sum = 0.0;
      std::int64_t train_total_rows = 0;
      std::int64_t batch_index = 0;
      for (const auto& raw_batch : learn_batches) {
        if (max_train_batches > 0 && batch_index >= max_train_batches) break;
        state.stage = StepStage::Learn;
        state.batch_index = batch_index;
        state.rows = raw_batch.size();
        state.metrics.clear();
        callbacks.on_batch_begin(state);
        if (state.stop_requested) break;
        if (batch_index % accumulation_steps == 0) {
          optimizer->clear();
        }
        Batch batch = source.transfer_batch(
            raw_batch,
            SourceContext{
                .recipe = &options,
                .stage = StepStage::Learn,
                .target_platform = target_platform,
                .target_dtype = target_dtype,
            });
        const StepResult step = model.learn_step(
            batch,
            StepContext{
                .recipe = &options,
                .loss_kind = options.loss,
                .stage = StepStage::Learn,
                .epoch = epoch,
                .batch_index = batch_index,
            });
        PLUSS_CHECK(step.has_loss(),
                    "pluss::ai::ice::Coach::learn: learn_step must return a defined loss.");
        const std::int64_t rows = batch_rows_(batch);
        const double loss_value = step.loss.item<double>();
        train_weighted_loss_sum += loss_value * static_cast<double>(rows);
        train_total_rows += rows;
        state.rows = rows;
        state.metrics.clear();
        state.put_metric("loss", loss_value);
        callbacks.on_before_backward(state);
        if (state.stop_requested) break;
        const auto remaining_batches = max_train_batches > 0
                                           ? std::min<std::int64_t>(max_train_batches, static_cast<std::int64_t>(learn_batches.size()))
                                           : static_cast<std::int64_t>(learn_batches.size());
        const std::int64_t batches_left_in_epoch = remaining_batches - batch_index;
        const std::int64_t local_accumulation = std::min<std::int64_t>(accumulation_steps, batches_left_in_epoch);
        Series scaled_loss = step.loss.div(static_cast<double>(local_accumulation));
        scaled_loss.reverse();
        callbacks.on_after_backward(state);
        if (state.stop_requested) break;
        const bool should_step =
            ((batch_index + 1) % accumulation_steps == 0) || (batch_index + 1 == remaining_batches);
        if (should_step) {
          callbacks.on_before_step(state);
          if (state.stop_requested) break;
          optimizer->step();
          ++state.global_step;
        }
        callbacks.on_batch_end(state);
        if (state.stop_requested) break;
        ++batch_index;
      }
      const double train_loss_value =
          train_total_rows > 0 ? train_weighted_loss_sum / static_cast<double>(train_total_rows) : 0.0;
      if (train_total_rows > 0) {
        state.put_metric("loss", train_loss_value);
      }

      const bool should_validate =
          has_val && epoch >= options.validation.start_from_epoch &&
          ((epoch - options.validation.start_from_epoch) % options.validation.every_n_epochs == 0);
      if (should_validate) {
        model.eval();
        const NoGradGuard guard;
        double weighted_loss_sum = 0.0;
        std::int64_t total_rows = 0;
        std::int64_t valid_batch_index = 0;
        for (const auto& raw_batch : valid_batches) {
          if (max_valid_batches > 0 && valid_batch_index >= max_valid_batches) break;
          state.stage = StepStage::Valid;
          state.batch_index = valid_batch_index;
          state.rows = raw_batch.size();
          state.metrics.clear();
          callbacks.on_batch_begin(state);
          if (state.stop_requested) break;
          Batch batch = source.transfer_batch(
              raw_batch,
              SourceContext{
                  .recipe = &options,
                  .stage = StepStage::Valid,
                  .target_platform = target_platform,
                  .target_dtype = target_dtype,
              });
          const StepResult step = model.valid_step(
              batch,
              StepContext{
                  .recipe = &options,
                  .loss_kind = options.loss,
                  .stage = StepStage::Valid,
                  .epoch = epoch,
                  .batch_index = valid_batch_index,
              });
          PLUSS_CHECK(step.has_loss(),
                      "pluss::ai::ice::Coach::learn: valid_step must return a defined loss.");
          const std::int64_t rows = batch_rows_(batch);
          const double loss_value = step.loss.item<double>();
          weighted_loss_sum += loss_value * static_cast<double>(rows);
          total_rows += rows;
          state.rows = rows;
          state.metrics.clear();
          state.put_metric("val_loss", loss_value);
          callbacks.on_batch_end(state);
          if (state.stop_requested) break;
          ++valid_batch_index;
        }
        PLUSS_CHECK(total_rows > 0,
                    "pluss::ai::ice::Coach::learn: source produced no validation rows.");
        state.stage = StepStage::Learn;
        state.metrics.clear();
        state.put_metric("loss", train_loss_value);
        state.put_metric("val_loss", weighted_loss_sum / static_cast<double>(total_rows));
        model.train(true);
      }
      callbacks.on_epoch_end(state);
      if (options.checkpoint.enabled && !state.stop_requested) {
        std::ostringstream rng_state_stream;
        rng_state_stream << rng;
        io::CheckpointState checkpoint_state{
            .recipe = options,
            .next_epoch = epoch + 1,
            .global_step = state.global_step,
            .best_metric = state.best_metric,
            .best_metric_name = state.best_metric_name,
            .best_checkpoint_path = state.best_checkpoint_path,
            .last_checkpoint_path = state.last_checkpoint_path,
            .stop_reason = state.stop_reason,
            .rng_state = rng_state_stream.str(),
        };
        if (options.checkpoint.save_best) {
          const auto monitor_value = state.metric(options.validation.monitor);
          PLUSS_CHECK(monitor_value.has_value(),
                      "pluss::ai::ice::Coach::learn: checkpoint.save_best requires the monitored metric '" +
                          options.validation.monitor + "' to be produced each checkpointed epoch.");
          const bool improved = monitor_value.has_value() &&
              (!state.best_metric.has_value() ||
               monitor_improved_(*monitor_value, *state.best_metric, options.validation,
                                 options.loop_regularization.min_delta));
          if (improved) {
            state.best_metric = *monitor_value;
            state.best_metric_name = options.validation.monitor;
            state.best_checkpoint_path = checkpoint_path_(options.checkpoint, "-best.ckpt");
            checkpoint_state.best_metric = state.best_metric;
            checkpoint_state.best_metric_name = state.best_metric_name;
            checkpoint_state.best_checkpoint_path = state.best_checkpoint_path;
            maybe_save_checkpoint_(options.checkpoint, model, *optimizer, state, callbacks, checkpoint_state,
                                   state.best_checkpoint_path);
          }
        }
        if (options.checkpoint.save_last &&
            ((epoch + 1) % options.checkpoint.every_n_epochs == 0)) {
          state.last_checkpoint_path = checkpoint_path_(options.checkpoint, "-last.ckpt");
          checkpoint_state.best_metric = state.best_metric;
          checkpoint_state.best_metric_name = state.best_metric_name;
          checkpoint_state.best_checkpoint_path = state.best_checkpoint_path;
          checkpoint_state.last_checkpoint_path = state.last_checkpoint_path;
          maybe_save_checkpoint_(options.checkpoint, model, *optimizer, state, callbacks, checkpoint_state,
                                 state.last_checkpoint_path);
        }
      }
      if (state.stop_requested) break;
      if (options.execution.fast_dev_batches > 0) break;
    }
    callbacks.on_run_end(state);
  } catch (const std::exception& e) {
    state.exception_message = e.what();
    callbacks.on_exception(state);
    if (options.checkpoint.enabled && options.checkpoint.save_on_exception) {
      std::ostringstream rng_state_stream;
      rng_state_stream << rng;
      const auto exception_path = checkpoint_path_(options.checkpoint, "-exception.ckpt");
      io::CheckpointState checkpoint_state{
          .recipe = options,
          .next_epoch = std::max<std::int64_t>(0, state.epoch),
          .global_step = state.global_step,
          .best_metric = state.best_metric,
          .best_metric_name = state.best_metric_name,
          .best_checkpoint_path = state.best_checkpoint_path,
          .last_checkpoint_path = state.last_checkpoint_path,
          .stop_reason = "exception",
          .rng_state = rng_state_stream.str(),
      };
      maybe_save_checkpoint_(options.checkpoint, model, *optimizer, state, callbacks, checkpoint_state,
                             exception_path);
    }
    PLUSS_FAIL(std::string("pluss::ai::ice::Coach::learn: training became numerically unstable: ") +
               e.what());
  }

  model.eval();
  model.trained_ = true;
}

AssessReport Coach::assess_source_(const Model& model,
                                   Source& source,
                                   LossKind loss_kind) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(model.trained_, "pluss::ai::ice::Coach::assess: model has not been trained.");
  source.prepare();
  const auto& test_batches = source.test_batches();
  PLUSS_CHECK(!test_batches.empty(),
              "pluss::ai::ice::Coach::assess: source produced no test batches.");

  const auto parameters = model.all_parameters_();
  const bool was_training = model.is_training();
  const_cast<Model&>(model).eval();
  const NoGradGuard guard;
  const Platform model_platform = parameters.front()->platform();
  const ScalarType model_dtype = parameters.front()->scalar_type();
  std::vector<Series> predictions{};
  std::vector<Series> targets{};
  predictions.reserve(test_batches.size());
  targets.reserve(test_batches.size());
  RunState state{
      .model = const_cast<Model*>(&model),
      .stage = StepStage::Test,
      .target_platform = model_platform,
      .epoch = -1,
      .batch_index = -1,
      .global_step = 0,
      .rows = 0,
      .metrics = {},
      .parameters = &parameters,
      .stop_requested = false,
      .stop_reason = {},
      .exception_message = {},
  };
  double weighted_loss_sum = 0.0;
  std::int64_t total_rows = 0;
  std::int64_t batch_index = 0;
  CallbackChain callbacks(callbacks_);
  try {
    callbacks.on_run_begin(state);
    for (const auto& raw_batch : test_batches) {
      state.batch_index = batch_index;
      state.rows = raw_batch.size();
      state.metrics.clear();
      callbacks.on_batch_begin(state);
      if (state.stop_requested) break;
      Batch batch = source.transfer_batch(
          raw_batch,
          SourceContext{
              .recipe = nullptr,
              .stage = StepStage::Test,
              .target_platform = model_platform,
              .target_dtype = model_dtype,
          });
      PLUSS_CHECK(batch.has_targets(),
                  "pluss::ai::ice::Coach::assess: transferred test batch must contain targets.");
      const StepResult step = model.test_step(
          batch,
          StepContext{
              .recipe = nullptr,
              .loss_kind = loss_kind,
              .stage = StepStage::Test,
              .epoch = -1,
              .batch_index = batch_index,
          });
      PLUSS_CHECK(step.has_predictions() && step.has_loss(),
                  "pluss::ai::ice::Coach::assess: test_step must return predictions and loss.");
      const std::int64_t rows = batch_rows_(batch);
      const double loss_value = step.loss.item<double>();
      weighted_loss_sum += loss_value * static_cast<double>(rows);
      total_rows += rows;
      predictions.push_back(step.predictions);
      targets.push_back(batch.targets());
      state.rows = rows;
      state.metrics.clear();
      state.put_metric("test_loss", loss_value);
      callbacks.on_batch_end(state);
      if (state.stop_requested) break;
      ++batch_index;
    }
  } catch (const std::exception& e) {
    state.exception_message = e.what();
    callbacks.on_exception(state);
    PLUSS_FAIL(std::string("pluss::ai::ice::Coach::assess: evaluation became numerically unstable: ") +
               e.what());
  }
  PLUSS_CHECK(total_rows > 0,
              "pluss::ai::ice::Coach::assess: source produced no test rows.");
  const Series all_predictions = predictions.size() == 1 ? predictions.front() : pluss::series::unite(predictions, 0);
  const Series all_targets = targets.size() == 1 ? targets.front() : pluss::series::unite(targets, 0);

  AssessReport report{};
  report.loss = weighted_loss_sum / static_cast<double>(total_rows);

  switch (loss_kind) {
    case LossKind::MSE: {
      const Series diff = all_predictions.sub(all_targets);
      const Series mse = diff.mul(diff).mean();
      report.metric = mse.sqrt().item<double>();
      report.metric_name = "rmse";
      break;
    }
    case LossKind::BCEWithLogits: {
      const Series target = all_targets.to(ScalarType::Float32);
      const Series predicted = all_predictions.sigmoid().ge(0.5).to(ScalarType::Float32);
      const Series correct = predicted.eq(target).to(ScalarType::Float32);
      report.metric = correct.mean().item<double>();
      report.metric_name = "accuracy";
      break;
    }
    case LossKind::CrossEntropy: {
      const Series target = all_targets.to(ScalarType::Float32);
      const Series predicted =
          all_predictions.argmax(all_predictions.sizes().size() - 1).to(ScalarType::Float32);
      const Series correct = predicted.eq(target).to(ScalarType::Float32);
      report.metric = correct.mean().item<double>();
      report.metric_name = "accuracy";
      break;
    }
  }
  state.metrics.clear();
  state.put_metric("test_loss", report.loss);
  state.put_metric(report.metric_name, report.metric);
  callbacks.on_run_end(state);
  const_cast<Model&>(model).train(was_training);
  return report;
}

Series Coach::infer_source_(const Model& model, Source& source) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(model.trained_, "pluss::ai::ice::Coach::infer: model has not been trained.");
  source.prepare();
  const auto& infer_batches = source.infer_batches();
  PLUSS_CHECK(!infer_batches.empty(),
              "pluss::ai::ice::Coach::infer: source produced no infer batches.");

  const auto parameters = model.all_parameters_();
  const bool was_training = model.is_training();
  const_cast<Model&>(model).eval();
  const Platform model_platform = parameters.front()->platform();
  const ScalarType model_dtype = parameters.front()->scalar_type();
  std::vector<Series> chunks{};
  chunks.reserve(infer_batches.size());
  const NoGradGuard guard;
  RunState state{
      .model = const_cast<Model*>(&model),
      .stage = StepStage::Infer,
      .target_platform = model_platform,
      .epoch = -1,
      .batch_index = -1,
      .global_step = 0,
      .rows = 0,
      .metrics = {},
      .parameters = &parameters,
      .stop_requested = false,
      .stop_reason = {},
      .exception_message = {},
  };
  std::int64_t batch_index = 0;
  CallbackChain callbacks(callbacks_);
  try {
    callbacks.on_run_begin(state);
    for (const auto& raw_batch : infer_batches) {
      state.batch_index = batch_index;
      state.rows = raw_batch.size();
      state.metrics.clear();
      callbacks.on_batch_begin(state);
      if (state.stop_requested) break;
      Batch batch = source.transfer_batch(
          raw_batch,
          SourceContext{
              .recipe = nullptr,
              .stage = StepStage::Infer,
              .target_platform = model_platform,
              .target_dtype = model_dtype,
          });
      const StepResult step = model.infer_step(
          batch,
          StepContext{
              .recipe = nullptr,
              .loss_kind = std::nullopt,
              .stage = StepStage::Infer,
              .epoch = -1,
              .batch_index = batch_index,
          });
      PLUSS_CHECK(step.has_predictions(),
                  "pluss::ai::ice::Coach::infer: infer_step must return predictions.");
      chunks.push_back(step.predictions);
      state.rows = batch_rows_(batch);
      callbacks.on_batch_end(state);
      if (state.stop_requested) break;
      ++batch_index;
    }
    callbacks.on_run_end(state);
  } catch (const std::exception& e) {
    state.exception_message = e.what();
    callbacks.on_exception(state);
    PLUSS_FAIL(std::string("pluss::ai::ice::Coach::infer: inference became numerically unstable: ") +
               e.what());
  }
  const_cast<Model&>(model).train(was_training);
  return chunks.size() == 1 ? chunks.front() : pluss::series::unite(chunks, 0);
}

void Coach::learn(Model& model, Source& source) const {
  PLUSS_CALL_GUARD();
  train_loop_(model, source, options_);
}

void Coach::learn(Model& model, const Series& x_train, const Series& y_train) const {
  PLUSS_CALL_GUARD();
  if (options_.validation.split_ratio > 0.0) {
    const auto split = pluss::loader::split_fields(
        {x_train, y_train},
        pluss::loader::DataSplitOptions{
            .train_ratio = 1.0 - options_.validation.split_ratio,
            .val_ratio = options_.validation.split_ratio,
            .shuffle = options_.shuffle,
            .seed = options_.seed,
        });
    PLUSS_CHECK(split.val_fields[0].numel() > 0,
                "pluss::ai::ice::Coach::learn: validation.split_ratio left no validation rows; "
                "use more rows or set validation.split_ratio to 0.");
    TensorSource source(
        split.train_fields[0],
        split.train_fields[1],
        split.val_fields[0],
        split.val_fields[1],
        BatchPlan{
            .batch_size = options_.batch_size,
            .shuffle = options_.shuffle,
            .seed = options_.seed,
        });
    learn(model, source);
  } else {
    TensorSource source(
        x_train,
        y_train,
        std::nullopt,
        std::nullopt,
        BatchPlan{
            .batch_size = options_.batch_size,
            .shuffle = options_.shuffle,
            .seed = options_.seed,
        });
    learn(model, source);
  }
}

void Coach::learn(Model& model,
                  const Series& x_train,
                  const Series& y_train,
                  const Series& x_val,
                  const Series& y_val) const {
  PLUSS_CALL_GUARD();
  TensorSource source(
      x_train,
      y_train,
      x_val,
      y_val,
      BatchPlan{
          .batch_size = options_.batch_size,
          .shuffle = options_.shuffle,
          .seed = options_.seed,
      });
  learn(model, source);
}

void Coach::learn(Model& model, const pluss::table::DataFrame& df, std::string_view target) const {
  PLUSS_CALL_GUARD();
  Series x = model.dataframe_matrix_(df, target);
  Series y = df.features(target).as_series().to(pluss::pCPU, ScalarType::Float32);
  if (y.sizes().size() == 1) {
    y = y.reshape({y.numel(), 1});
  }
  if (options_.standardize) {
    x = model.fit_standardize_(x);
  }
  learn(model, x, y);
}

Series Coach::infer(const Model& model, Source& source) const {
  return infer_source_(model, source);
}

Series Coach::infer(const Model& model, const Series& x, std::int64_t chunk_size) const {
  TensorSource source(
      x,
      BatchPlan{
          .batch_size = chunk_size,
          .shuffle = false,
          .seed = 0,
      });
  return infer(model, source);
}

Series Coach::infer(const Model& model,
                    const pluss::table::DataFrame& df,
                    std::string_view target,
                    std::int64_t chunk_size) const {
  PLUSS_CALL_GUARD();
  const Series x = model.apply_standardize_(model.dataframe_matrix_(df, target));
  return infer(model, x, chunk_size);
}

AssessReport Coach::assess(const Model& model,
                           Source& source,
                           LossKind loss) const {
  return assess_source_(model, source, loss);
}

AssessReport Coach::assess(const Model& model,
                           const Series& x,
                           const Series& y,
                           LossKind loss) const {
  TensorSource source(x, y);
  return assess(model, source, loss);
}

AssessReport Coach::assess(const Model& model,
                           const pluss::table::DataFrame& df,
                           std::string_view target,
                           LossKind loss) const {
  PLUSS_CALL_GUARD();
  const Series x = model.apply_standardize_(model.dataframe_matrix_(df, target));
  Series y = df.features(target).as_series().to(pluss::pCPU, ScalarType::Float32);
  if (y.sizes().size() == 1) {
    y = y.reshape({y.numel(), 1});
  }
  return assess(model, x, y, loss);
}

} // namespace pluss::ai::ice
