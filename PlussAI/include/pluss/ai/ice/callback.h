#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/ai/ice/step.h"
#include "pluss/nn.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::ice {

class Model;

struct RunState {
  Model* model = nullptr;
  StepStage stage = StepStage::Infer;
  Platform target_platform = pluss::pCPU;
  std::int64_t epoch = -1;
  std::int64_t batch_index = -1;
  std::int64_t global_step = 0;
  std::int64_t rows = 0;
  std::unordered_map<std::string, double> metrics{};
  const std::vector<Series*>* parameters = nullptr;
  const TrainOptions* recipe = nullptr;
  bool stop_requested = false;
  std::string stop_reason{};
  std::string exception_message{};
  std::string checkpoint_path{};
  std::string best_checkpoint_path{};
  std::string last_checkpoint_path{};
  std::optional<double> best_metric = std::nullopt;
  std::string best_metric_name{};

  [[nodiscard]] std::optional<double> metric(std::string_view name) const;
  void put_metric(std::string name, double value);
};

class Callback {
public:
  virtual ~Callback() = default;

  virtual void on_run_begin(RunState& state);
  virtual void on_run_end(RunState& state);
  virtual void on_epoch_begin(RunState& state);
  virtual void on_epoch_end(RunState& state);
  virtual void on_batch_begin(RunState& state);
  virtual void on_batch_end(RunState& state);
  virtual void on_before_backward(RunState& state);
  virtual void on_after_backward(RunState& state);
  virtual void on_before_step(RunState& state);
  virtual void on_checkpoint_save(RunState& state, std::string_view filepath);
  virtual void on_checkpoint_load(RunState& state, std::string_view filepath);
  virtual void on_exception(RunState& state);
  [[nodiscard]] virtual std::string state_key() const;
  [[nodiscard]] virtual std::unordered_map<std::string, std::string> save_state() const;
  virtual void load_state(const std::unordered_map<std::string, std::string>& state);
};

class CallbackChain final : public Callback {
public:
  CallbackChain() = default;
  explicit CallbackChain(std::vector<std::shared_ptr<Callback>> callbacks);

  void add(std::shared_ptr<Callback> callback);
  [[nodiscard]] const std::vector<std::shared_ptr<Callback>>& callbacks() const noexcept { return callbacks_; }

  void on_run_begin(RunState& state) override;
  void on_run_end(RunState& state) override;
  void on_epoch_begin(RunState& state) override;
  void on_epoch_end(RunState& state) override;
  void on_batch_begin(RunState& state) override;
  void on_batch_end(RunState& state) override;
  void on_before_backward(RunState& state) override;
  void on_after_backward(RunState& state) override;
  void on_before_step(RunState& state) override;
  void on_checkpoint_save(RunState& state, std::string_view filepath) override;
  void on_checkpoint_load(RunState& state, std::string_view filepath) override;
  void on_exception(RunState& state) override;

private:
  std::vector<std::shared_ptr<Callback>> callbacks_{};
};

class EarlyStopCallback final : public Callback {
public:
  EarlyStopCallback(ValidationPolicy validation, LoopRegularization regularization)
      : validation_(std::move(validation)),
        regularization_(std::move(regularization)) {}

  void on_run_begin(RunState& state) override;
  void on_epoch_end(RunState& state) override;
  void on_run_end(RunState& state) override;
  [[nodiscard]] std::string state_key() const override;
  [[nodiscard]] std::unordered_map<std::string, std::string> save_state() const override;
  void load_state(const std::unordered_map<std::string, std::string>& state) override;

  [[nodiscard]] const std::string& monitor() const noexcept { return validation_.monitor; }
  [[nodiscard]] std::optional<double> best_value() const noexcept { return best_value_; }
  [[nodiscard]] std::int64_t best_epoch() const noexcept { return best_epoch_; }
  [[nodiscard]] std::int64_t stopped_epoch() const noexcept { return stopped_epoch_; }
  [[nodiscard]] std::int64_t wait_count() const noexcept { return wait_count_; }

private:
  ValidationPolicy validation_{};
  LoopRegularization regularization_{};
  std::optional<double> best_value_ = std::nullopt;
  std::int64_t best_epoch_ = -1;
  std::int64_t stopped_epoch_ = -1;
  std::int64_t wait_count_ = 0;
  std::vector<Series> best_parameters_{};
  bool loaded_from_checkpoint_ = false;
};

} // namespace pluss::ai::ice
