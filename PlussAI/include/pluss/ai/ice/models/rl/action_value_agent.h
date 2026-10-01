#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/ai/ice/platform.h"
#include "pluss/pluss.h"

#include <cstdint>
#include <memory>
#include <string_view>

namespace pluss::ai::ice::models {

struct ActionValueAgentOptions {
  std::int64_t observation_size = 0;
  std::int64_t action_count = 0;
  std::int64_t hidden_size = 64;
  std::int64_t hidden_layers = 2;
  double discount = 0.99;
  bool double_estimate = true;
  std::int64_t target_sync_every = 16;
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};

struct ActionValueTrainOptions {
  std::int64_t batch_size = 64;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  bool shuffle = true;
  bool use_huber = true;
  std::uint64_t seed = 0;
  ValidationPolicy validation{};
  ObjectiveRegularization objective_regularization{};
  LoopRegularization loop_regularization{};
  ExecutionPolicy execution{};
};

struct ActionValueBatch {
  Series observation = pluss::series::undefined();
  Series action = pluss::series::undefined();
  Series reward = pluss::series::undefined();
  Series next_observation = pluss::series::undefined();
  Series done = pluss::series::undefined();
};

struct ActionValueReport {
  double loss = 0.0;
  double mean_q = 0.0;
  std::int64_t count = 0;
};

class ActionValueAgent final {
public:
  struct Impl;

  explicit ActionValueAgent(ActionValueAgentOptions options);

  void learn(const ActionValueBatch& train,
             const ActionValueTrainOptions& options = {});
  void learn(const ActionValueBatch& train,
             const ActionValueBatch& val,
             const ActionValueTrainOptions& options = {});

  [[nodiscard]] Series predict(const Series& observations) const;
  [[nodiscard]] std::int64_t act(const Series& observation,
                                 double epsilon = 0.0,
                                 std::uint64_t seed = 0) const;
  [[nodiscard]] ActionValueReport assess(const ActionValueBatch& batch,
                                         bool use_huber = true) const;

  void save(std::string_view filepath) const;
  [[nodiscard]] static ActionValueAgent load(std::string_view filepath);

  [[nodiscard]] bool ready() const noexcept;
  ActionValueAgent& to(Platform platform);

private:
  std::shared_ptr<Impl> impl_;
  ActionValueAgentOptions options_{};
};

} // namespace pluss::ai::ice::models
