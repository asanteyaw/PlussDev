#include "pluss/ai/ice/models/rl/action_value_agent.h"

#include "pluss/ai/ice/io.h"
#include "pluss/ai/ice/callback.h"
#include "pluss/ai/ice/io/kv_artifact.h"
#include "pluss/ai/ice/no_grad.h"
#include "pluss/ai/ice/optim.h"
#include "pluss/ai/fusion.h"
#include "pluss/nn.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::ice::models {

namespace {


constexpr std::string_view kMagic = "pluss_ice_action_value_agent";
constexpr std::int64_t kSchema = 1;

[[nodiscard]] TrainOptions recipe_(const ActionValueTrainOptions& options) {
  TrainOptions out{};
  out.batch_size = options.batch_size;
  out.learning_rate = options.learning_rate;
  out.optimizer = options.optimizer;
  out.shuffle = options.shuffle;
  out.seed = options.seed;
  out.progress = options.progress;
  out.validation = options.validation;
  out.objective_regularization = options.objective_regularization;
  out.loop_regularization = options.loop_regularization;
  out.execution = options.execution;
  return out;
}

[[nodiscard]] std::vector<Series> clone_parameters_(const std::vector<Series*>& parameters) {
  std::vector<Series> out{};
  out.reserve(parameters.size());
  for (auto* parameter : parameters) out.push_back(parameter->clone());
  return out;
}

void restore_parameters_(const std::vector<Series*>& parameters,
                         const std::vector<Series>& snapshot) {
  PLUSS_CHECK(parameters.size() == snapshot.size(),
              "pluss::ai::ice::models::ActionValueAgent: parameter snapshot mismatch.");
  for (std::size_t i = 0; i < parameters.size(); ++i) {
    parameters[i]->copy_(snapshot[i]);
  }
}

[[nodiscard]] Series apply_penalties_(const Series& base_loss,
                                      const std::vector<Series*>& parameters,
                                      const ObjectiveRegularization& options) {
  Series out = base_loss;
  if (options.l1_penalty > 0.0) {
    for (auto* parameter : parameters) {
      out = out.add(parameter->abs().mean().mul(options.l1_penalty));
    }
  }
  if (options.l2_penalty > 0.0) {
    for (auto* parameter : parameters) {
      out = out.add(parameter->square().mean().mul(options.l2_penalty));
    }
  }
  return out;
}

template <class T>
void append_parameters_(std::vector<Series*>& out, T& module) {
  const auto more = module->parameters();
  out.insert(out.end(), more.begin(), more.end());
}

using pluss::ai::ice::io::detail::parse_kv_body;
using pluss::ai::ice::io::detail::require_bool;
using pluss::ai::ice::io::detail::require_f64;
using pluss::ai::ice::io::detail::require_field;
using pluss::ai::ice::io::detail::require_i64;
using pluss::ai::ice::io::detail::require_u64;
using pluss::ai::ice::io::detail::write_kv_line;
using pluss::ai::ice::io::detail::write_num_line;

constexpr std::string_view kLoadContext = "pluss::ai::ice::models::ActionValueAgent::load";

[[nodiscard]] std::vector<std::int64_t> split_indices_(std::int64_t count,
                                                       double holdout_ratio,
                                                       bool shuffle,
                                                       std::uint64_t seed,
                                                       bool tail) {
  std::vector<std::int64_t> order(static_cast<std::size_t>(count));
  std::iota(order.begin(), order.end(), 0);
  if (shuffle) {
    std::mt19937_64 rng(seed);
    std::shuffle(order.begin(), order.end(), rng);
  }
  const auto holdout =
      std::clamp<std::int64_t>(static_cast<std::int64_t>(std::llround(static_cast<double>(count) * holdout_ratio)),
                               0, count - 1);
  if (holdout <= 0) return tail ? std::vector<std::int64_t>{} : order;
  if (tail) {
    return std::vector<std::int64_t>(order.end() - holdout, order.end());
  }
  return std::vector<std::int64_t>(order.begin(), order.end() - holdout);
}

[[nodiscard]] Series index_series_(const std::vector<std::int64_t>& indices) {
  return pluss::make_series(indices,
                            {static_cast<std::int64_t>(indices.size())},
                            Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
}

[[nodiscard]] Series as_vector_(const Series& value, std::string_view what) {
  PLUSS_CHECK(value.defined(),
              "pluss::ai::ice::models::ActionValueAgent: " + std::string(what) + " is undefined.");
  if (value.dim() == 1) return value;
  PLUSS_CHECK(value.dim() == 2 && value.sizes()[1] == 1,
              "pluss::ai::ice::models::ActionValueAgent: " + std::string(what) +
                  " must be rank 1 or rank 2 with one trailing column.");
  return value.reshape({value.sizes()[0]});
}

void validate_model_options_(const ActionValueAgentOptions& options) {
  PLUSS_CHECK(options.observation_size > 0 && options.action_count > 1 &&
                  options.hidden_size > 0 && options.hidden_layers > 0,
              "pluss::ai::ice::models::ActionValueAgent: observation_size, action_count, "
              "hidden_size, and hidden_layers must be positive; action_count must exceed 1.");
  PLUSS_CHECK(options.discount >= 0.0 && options.discount <= 1.0,
              "pluss::ai::ice::models::ActionValueAgent: discount must be in [0, 1].");
  PLUSS_CHECK(options.target_sync_every > 0,
              "pluss::ai::ice::models::ActionValueAgent: target_sync_every must be positive.");
}

void validate_train_options_(const ActionValueTrainOptions& options) {
  PLUSS_CHECK(options.batch_size > 0 && options.learning_rate > 0.0 &&
                  options.execution.max_epochs > 0 && options.validation.every_n_epochs > 0,
              "pluss::ai::ice::models::ActionValueAgent: batch_size, learning_rate, "
              "execution.max_epochs, and validation.every_n_epochs must be positive.");
}

void validate_batch_(const ActionValueBatch& batch,
                     const ActionValueAgentOptions& options,
                     const char* what) {
  PLUSS_CHECK(batch.observation.defined() && batch.next_observation.defined() &&
                  batch.action.defined() && batch.reward.defined() && batch.done.defined(),
              std::string(what) + ": every field must be defined.");
  PLUSS_CHECK(batch.observation.dim() == 2 &&
                  batch.observation.sizes()[1] == options.observation_size,
              std::string(what) + ": observation must be [N, observation_size].");
  PLUSS_CHECK(batch.next_observation.dim() == 2 &&
                  batch.next_observation.sizes()[1] == options.observation_size,
              std::string(what) + ": next_observation must be [N, observation_size].");
  const auto n = batch.observation.sizes()[0];
  PLUSS_CHECK(batch.next_observation.sizes()[0] == n,
              std::string(what) + ": observation and next_observation row counts must match.");
  PLUSS_CHECK(as_vector_(batch.action, "action").sizes()[0] == n &&
                  as_vector_(batch.reward, "reward").sizes()[0] == n &&
                  as_vector_(batch.done, "done").sizes()[0] == n,
              std::string(what) + ": action, reward, and done must each have one value per row.");
}

[[nodiscard]] ActionValueBatch index_select_batch_(const ActionValueBatch& batch,
                                                   const std::vector<std::int64_t>& indices) {
  const Series index = index_series_(indices);
  return ActionValueBatch{
      .observation = batch.observation.index_select(0, index),
      .action = as_vector_(batch.action, "action").index_select(0, index),
      .reward = as_vector_(batch.reward, "reward").index_select(0, index),
      .next_observation = batch.next_observation.index_select(0, index),
      .done = as_vector_(batch.done, "done").index_select(0, index),
  };
}

[[nodiscard]] ActionValueBatch move_batch_(const ActionValueBatch& batch, Platform platform) {
  return ActionValueBatch{
      .observation = batch.observation.to(platform, batch.observation.scalar_type()),
      .action = as_vector_(batch.action, "action").to(platform, ScalarType::Int64),
      .reward = as_vector_(batch.reward, "reward").to(platform, ScalarType::Float32),
      .next_observation = batch.next_observation.to(platform, batch.next_observation.scalar_type()),
      .done = as_vector_(batch.done, "done").to(platform, ScalarType::Float32),
  };
}

struct DuelingTower {
  std::vector<pluss::nn::Fusion> hidden{};
  std::vector<pluss::nn::ReLU> activations{};
  pluss::nn::Fusion value_head{nullptr};
  pluss::nn::Fusion advantage_head{nullptr};
  std::int64_t action_count = 0;

  DuelingTower() = default;

  explicit DuelingTower(const ActionValueAgentOptions& options,
                        std::uint64_t seed_offset) {
    std::int64_t in_features = options.observation_size;
    for (std::int64_t layer = 0; layer < options.hidden_layers; ++layer) {
      hidden.emplace_back(pluss::nn::FusionOptions{
          .in_features = in_features,
          .out_features = options.hidden_size,
          .bias = true,
          .cfg = options.cfg,
          .seed = options.seed + seed_offset + static_cast<std::uint64_t>(17 * layer + 1),
      });
      activations.emplace_back(pluss::nn::ReLU());
      in_features = options.hidden_size;
    }
    value_head.reset(pluss::nn::FusionOptions{
        .in_features = options.hidden_size,
        .out_features = 1,
        .bias = true,
        .cfg = options.cfg,
        .seed = options.seed + seed_offset + 1001ULL,
    });
    advantage_head.reset(pluss::nn::FusionOptions{
        .in_features = options.hidden_size,
        .out_features = options.action_count,
        .bias = true,
        .cfg = options.cfg,
        .seed = options.seed + seed_offset + 2001ULL,
    });
    action_count = options.action_count;
  }

  [[nodiscard]] Series forward(const Series& observation) const {
    Series h = observation;
    for (std::size_t i = 0; i < hidden.size(); ++i) {
      h = hidden[i]->forward(h);
      h = activations[i]->forward(h);
    }
    const Series value = value_head->forward(h);
    const Series advantage = advantage_head->forward(h);
    return value.add(advantage.sub(advantage.mean(1, true)));
  }

  [[nodiscard]] std::vector<Series*> parameters() {
    std::vector<Series*> out{};
    for (auto& layer : hidden) append_parameters_(out, layer);
    append_parameters_(out, value_head);
    append_parameters_(out, advantage_head);
    return out;
  }

  void train(bool enabled) {
    for (auto& layer : hidden) layer->train(enabled);
    value_head->train(enabled);
    advantage_head->train(enabled);
  }

  void to(Platform platform) {
    for (auto& layer : hidden) layer->to(platform);
    value_head->to(platform);
    advantage_head->to(platform);
  }
};

void sync_target_(DuelingTower& target, DuelingTower& online) {
  auto target_params = target.parameters();
  auto online_params = online.parameters();
  PLUSS_CHECK(target_params.size() == online_params.size(),
              "pluss::ai::ice::models::ActionValueAgent: target parameter mismatch.");
  for (std::size_t i = 0; i < target_params.size(); ++i) {
    target_params[i]->copy_(*online_params[i]);
  }
}

[[nodiscard]] Series td_target_(DuelingTower& online,
                                DuelingTower& target,
                                const ActionValueAgentOptions& model_options,
                                const ActionValueBatch& batch) {
  ice::NoGradGuard no_grad;
  const auto n = batch.next_observation.sizes()[0];
  Series next_value = pluss::series::undefined();
  if (model_options.double_estimate) {
    const Series online_next = online.forward(batch.next_observation);
    const Series best_action = online_next.argmax(1).to(ScalarType::Int64);
    const Series target_next = target.forward(batch.next_observation);
    next_value = target_next.gather(1, best_action.reshape({n, 1})).reshape({n});
  } else {
    next_value = target.forward(batch.next_observation).amax(1, false);
  }
  const Series alive = pluss::series::ones_like(batch.done, ScalarType::Float32).sub(batch.done);
  return batch.reward.add(alive.mul(model_options.discount).mul(next_value));
}

[[nodiscard]] std::pair<Series, Series> q_taken_and_target_(DuelingTower& online,
                                                            DuelingTower& target,
                                                            const ActionValueAgentOptions& model_options,
                                                            const ActionValueBatch& batch) {
  const auto n = batch.observation.sizes()[0];
  const Series q_values = online.forward(batch.observation);
  const Series q_taken = q_values.gather(1, batch.action.reshape({n, 1})).reshape({n});
  const Series target_value = td_target_(online, target, model_options, batch);
  return {q_taken, target_value};
}

[[nodiscard]] ActionValueReport assess_impl_(DuelingTower& online,
                                             DuelingTower& target,
                                             const ActionValueAgentOptions& model_options,
                                             const ActionValueBatch& batch,
                                             bool use_huber) {
  const auto [q_taken, target_value] = q_taken_and_target_(online, target, model_options, batch);
  const Series loss = use_huber ? pluss::nn::losses::Huber(q_taken, target_value)
                                : pluss::nn::losses::MSE(q_taken, target_value);
  return ActionValueReport{
      .loss = loss.to(pluss::pCPU, ScalarType::Float64).item<double>(),
      .mean_q = q_taken.mean().to(pluss::pCPU, ScalarType::Float64).item<double>(),
      .count = batch.observation.sizes()[0],
  };
}

} // namespace

struct ActionValueAgent::Impl {
  DuelingTower online{};
  DuelingTower target{};
  Platform platform = pluss::pCPU;
  std::int64_t update_steps = 0;
  bool trained = false;

  [[nodiscard]] std::vector<Series*> parameters() {
    return online.parameters();
  }
};

ActionValueAgent::ActionValueAgent(ActionValueAgentOptions options)
    : impl_(std::make_shared<Impl>()),
      options_(std::move(options)) {
  validate_model_options_(options_);
  impl_->platform = options_.cfg.platform;
  impl_->online = DuelingTower(options_, 0ULL);
  impl_->target = DuelingTower(options_, 100000ULL);
  sync_target_(impl_->target, impl_->online);
}

void ActionValueAgent::learn(const ActionValueBatch& train,
                             const ActionValueTrainOptions& options) {
  validate_batch_(train, options_, "pluss::ai::ice::models::ActionValueAgent::learn");
  const auto train_ids = split_indices_(train.observation.sizes()[0],
                                        options.validation.split_ratio,
                                        options.shuffle,
                                        options.seed,
                                        /*tail=*/false);
  const auto val_ids = split_indices_(train.observation.sizes()[0],
                                      options.validation.split_ratio,
                                      options.shuffle,
                                      options.seed,
                                      /*tail=*/true);
  const bool use_validation = !val_ids.empty() &&
                              options.validation.split_ratio > 0.0 &&
                              static_cast<std::int64_t>(val_ids.size()) < train.observation.sizes()[0];
  if (use_validation) {
    learn(index_select_batch_(train, train_ids), index_select_batch_(train, val_ids), options);
    return;
  }
  learn(train, ActionValueBatch{}, options);
}

void ActionValueAgent::learn(const ActionValueBatch& train,
                             const ActionValueBatch& val,
                             const ActionValueTrainOptions& options) {
  PLUSS_CALL_GUARD();
  validate_model_options_(options_);
  validate_train_options_(options);
  validate_batch_(train, options_, "pluss::ai::ice::models::ActionValueAgent::learn(train)");
  const bool use_validation = val.observation.defined();
  if (use_validation) {
    validate_batch_(val, options_, "pluss::ai::ice::models::ActionValueAgent::learn(val)");
  }

  impl_ = std::make_shared<Impl>();
  options_.cfg = options_.cfg.with_platform(resolve_platform(options.execution.platform));
  impl_->platform = options_.cfg.platform;
  impl_->online = DuelingTower(options_, 0ULL);
  impl_->target = DuelingTower(options_, 100000ULL);
  impl_->online.to(impl_->platform);
  impl_->target.to(impl_->platform);
  sync_target_(impl_->target, impl_->online);

  auto parameters = impl_->parameters();
  const TrainOptions runtime_options = recipe_(options);
  auto optimizer = make_optimizer(parameters, runtime_options);

  ActionValueBatch train_batch = move_batch_(train, impl_->platform);
  ActionValueBatch val_batch = use_validation ? move_batch_(val, impl_->platform) : ActionValueBatch{};

  std::optional<std::vector<Series>> best_parameters = std::nullopt;
  double best_metric = std::numeric_limits<double>::infinity();
  std::int64_t stale = 0;
  std::vector<std::int64_t> order(static_cast<std::size_t>(train_batch.observation.sizes()[0]));
  std::iota(order.begin(), order.end(), 0);
  CallbackChain callbacks{};
  if (options.progress.verbose) {
    callbacks.add(std::make_shared<EpochLoggerCallback>(options.progress));
  }
  RunState state{
      .model = nullptr,
      .stage = StepStage::Learn,
      .target_platform = impl_->platform,
      .epoch = -1,
      .batch_index = -1,
      .global_step = 0,
      .rows = 0,
      .metrics = {},
      .parameters = &parameters,
      .recipe = &runtime_options,
  };

  try {
    callbacks.on_run_begin(state);
    for (std::int64_t epoch = 0; epoch < options.execution.max_epochs; ++epoch) {
      state.epoch = epoch;
      state.batch_index = -1;
      state.metrics.clear();
      callbacks.on_epoch_begin(state);
      double train_weighted_loss_sum = 0.0;
      std::int64_t train_total_rows = 0;
      if (options.shuffle) {
        std::mt19937_64 rng(options.seed + static_cast<std::uint64_t>(epoch));
        std::shuffle(order.begin(), order.end(), rng);
      }

      for (std::size_t begin = 0; begin < order.size(); begin += static_cast<std::size_t>(options.batch_size)) {
        const auto end = std::min(order.size(), begin + static_cast<std::size_t>(options.batch_size));
        std::vector<std::int64_t> batch_ids(order.begin() + static_cast<std::ptrdiff_t>(begin),
                                            order.begin() + static_cast<std::ptrdiff_t>(end));
        auto batch = index_select_batch_(train_batch, batch_ids);
        batch = move_batch_(batch, impl_->platform);

        impl_->online.train(true);
        impl_->target.train(false);
        const auto [q_taken, target_value] = q_taken_and_target_(impl_->online, impl_->target, options_, batch);
        Series loss = options.use_huber ? pluss::nn::losses::Huber(q_taken, target_value)
                                        : pluss::nn::losses::MSE(q_taken, target_value);
        loss = apply_penalties_(loss, parameters, options.objective_regularization);

        if (options.progress.verbose) {
          const auto rows = static_cast<std::int64_t>(end - begin);
          train_weighted_loss_sum += loss.item<double>() * static_cast<double>(rows);
          train_total_rows += rows;
        }

        optimizer->clear();
        loss.reverse();
        optimizer->step();

        ++impl_->update_steps;
        ++state.global_step;
        if (impl_->update_steps % options_.target_sync_every == 0) {
          sync_target_(impl_->target, impl_->online);
        }
      }

      state.metrics.clear();
      if (train_total_rows > 0) {
        state.put_metric("loss", train_weighted_loss_sum / static_cast<double>(train_total_rows));
      }

      const bool should_validate =
          use_validation && epoch >= options.validation.start_from_epoch &&
          ((epoch - options.validation.start_from_epoch) % options.validation.every_n_epochs == 0);
      if (should_validate) {
        impl_->online.train(false);
        impl_->target.train(false);
        const auto report = assess_impl_(impl_->online, impl_->target, options_, val_batch, options.use_huber);
        state.put_metric("val_loss", report.loss);
        state.put_metric("mean_q", report.mean_q);
        if (report.loss + options.loop_regularization.min_delta < best_metric) {
          best_metric = report.loss;
          stale = 0;
          if (options.loop_regularization.restore_best_weights) {
            best_parameters = clone_parameters_(parameters);
          }
        } else {
          ++stale;
          if (options.loop_regularization.early_stopping &&
              stale >= options.loop_regularization.patience) {
            state.stop_requested = true;
            state.stop_reason = "early_stopping";
          }
        }
      }
      callbacks.on_epoch_end(state);
      if (state.stop_requested) break;
    }
    callbacks.on_run_end(state);
  } catch (const std::exception& e) {
    state.exception_message = e.what();
    callbacks.on_exception(state);
    PLUSS_FAIL(std::string("pluss::ai::ice::models::ActionValueAgent::learn: training became numerically unstable: ") +
               e.what());
  }

  if (best_parameters.has_value()) {
    restore_parameters_(parameters, *best_parameters);
    sync_target_(impl_->target, impl_->online);
  }
  for (auto* parameter : parameters) {
    parameter->tracks_slope_(false);
    parameter->clear();
  }
  for (auto* parameter : impl_->target.parameters()) {
    parameter->tracks_slope_(false);
    parameter->clear();
  }
  impl_->online.train(false);
  impl_->target.train(false);
  impl_->trained = true;
}

Series ActionValueAgent::predict(const Series& observations) const {
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::ActionValueAgent::predict: model has not been trained.");
  PLUSS_CHECK(observations.defined() && observations.dim() == 2 &&
                  observations.sizes()[1] == options_.observation_size,
              "pluss::ai::ice::models::ActionValueAgent::predict: observations must be [N, observation_size].");
  ice::NoGradGuard no_grad;
  auto input = observations.to(impl_->platform, observations.scalar_type());
  return impl_->online.forward(input);
}

std::int64_t ActionValueAgent::act(const Series& observation,
                                   double epsilon,
                                   std::uint64_t seed) const {
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::ActionValueAgent::act: model has not been trained.");
  PLUSS_CHECK(epsilon >= 0.0 && epsilon <= 1.0,
              "pluss::ai::ice::models::ActionValueAgent::act: epsilon must be in [0, 1].");
  Series row = observation;
  if (row.dim() == 1) {
    PLUSS_CHECK(row.sizes()[0] == options_.observation_size,
                "pluss::ai::ice::models::ActionValueAgent::act: observation width mismatch.");
    row = row.reshape({1, options_.observation_size});
  } else {
    PLUSS_CHECK(row.dim() == 2 && row.sizes()[0] == 1 &&
                    row.sizes()[1] == options_.observation_size,
                "pluss::ai::ice::models::ActionValueAgent::act: observation must be [observation_size] or [1, observation_size].");
  }
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> chance(0.0, 1.0);
  if (chance(rng) < epsilon) {
    std::uniform_int_distribution<std::int64_t> action_dist(0, options_.action_count - 1);
    return action_dist(rng);
  }
  const Series q = predict(row);
  return q.argmax(1).to(pluss::pCPU, ScalarType::Int64).item<std::int64_t>();
}

ActionValueReport ActionValueAgent::assess(const ActionValueBatch& batch,
                                           bool use_huber) const {
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::ActionValueAgent::assess: model has not been trained.");
  validate_batch_(batch, options_, "pluss::ai::ice::models::ActionValueAgent::assess");
  auto moved = move_batch_(batch, impl_->platform);
  return assess_impl_(impl_->online, impl_->target, options_, moved, use_huber);
}

void ActionValueAgent::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::ActionValueAgent::save: model has not been trained.");
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::ActionValueAgent::save: output path cannot be empty.");
  const auto online_path = std::string(filepath) + ".online.weights";
  const auto target_path = std::string(filepath) + ".target.weights";
  io::save_parameters(online_path, impl_->online.parameters());
  io::save_parameters(target_path, impl_->target.parameters());

  std::ostringstream out{};
  out << kMagic << '\n';
  write_num_line(out, "schema", kSchema);
  write_num_line(out, "observation_size", options_.observation_size);
  write_num_line(out, "action_count", options_.action_count);
  write_num_line(out, "hidden_size", options_.hidden_size);
  write_num_line(out, "hidden_layers", options_.hidden_layers);
  write_num_line(out, "discount", options_.discount);
  write_num_line(out, "double_estimate", options_.double_estimate ? 1 : 0);
  write_num_line(out, "target_sync_every", options_.target_sync_every);
  write_num_line(out, "cfg_dtype", static_cast<int>(options_.cfg.dtype));
  write_num_line(out, "cfg_platform", static_cast<int>(options_.cfg.platform));
  write_num_line(out, "seed", options_.seed);
  write_num_line(out, "update_steps", impl_->update_steps);
  write_kv_line(out, "online_path", online_path);
  write_kv_line(out, "target_path", target_path);

  std::ofstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::ActionValueAgent::save: failed to open '" + std::string(filepath) + "'.");
  file << out.str();
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::ActionValueAgent::save: failed while writing '" + std::string(filepath) + "'.");
}

ActionValueAgent ActionValueAgent::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::ActionValueAgent::load: input path cannot be empty.");
  std::ifstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::ActionValueAgent::load: failed to open '" + std::string(filepath) + "'.");
  std::string magic{};
  PLUSS_CHECK(std::getline(file, magic) && magic == kMagic,
              "pluss::ai::ice::models::ActionValueAgent::load: invalid model artifact header.");
  std::ostringstream body{};
  body << file.rdbuf();
  const auto kv = parse_kv_body(body.str(), kLoadContext);
  PLUSS_CHECK(require_i64(kv, "schema", kLoadContext) == kSchema,
              "pluss::ai::ice::models::ActionValueAgent::load: unsupported schema.");

  ActionValueAgentOptions options{};
  options.observation_size = require_i64(kv, "observation_size", kLoadContext);
  options.action_count = require_i64(kv, "action_count", kLoadContext);
  options.hidden_size = require_i64(kv, "hidden_size", kLoadContext);
  options.hidden_layers = require_i64(kv, "hidden_layers", kLoadContext);
  options.discount = require_f64(kv, "discount", kLoadContext);
  options.double_estimate = require_bool(kv, "double_estimate", kLoadContext);
  options.target_sync_every = require_i64(kv, "target_sync_every", kLoadContext);
  options.cfg = Config{}
                    .with_dtype(static_cast<ScalarType>(require_i64(kv, "cfg_dtype", kLoadContext)))
                    .with_platform(static_cast<Platform>(require_i64(kv, "cfg_platform", kLoadContext)));
  options.seed = require_u64(kv, "seed", kLoadContext);

  ActionValueAgent model(options);
  io::load_parameters(require_field(kv, "online_path", kLoadContext), model.impl_->online.parameters());
  io::load_parameters(require_field(kv, "target_path", kLoadContext), model.impl_->target.parameters());
  for (auto* parameter : model.impl_->online.parameters()) {
    parameter->tracks_slope_(false);
    parameter->clear();
  }
  for (auto* parameter : model.impl_->target.parameters()) {
    parameter->tracks_slope_(false);
    parameter->clear();
  }
  model.impl_->update_steps = require_i64(kv, "update_steps", kLoadContext);
  model.impl_->trained = true;
  model.impl_->online.train(false);
  model.impl_->target.train(false);
  return model;
}

bool ActionValueAgent::ready() const noexcept {
  return impl_ != nullptr && impl_->trained;
}

ActionValueAgent& ActionValueAgent::to(Platform platform) {
  options_.cfg = options_.cfg.with_platform(platform);
  if (!impl_) return *this;
  impl_->online.to(platform);
  impl_->target.to(platform);
  impl_->platform = platform;
  return *this;
}

} // namespace pluss::ai::ice::models
