#pragma once

#include "pluss/ai/nodej/dataset.h"
#include "MSharp/RangeView.h"

namespace pluss::ai::nodej {

class EstimatorBase {
public:
  virtual ~EstimatorBase() = default;

  [[nodiscard]] virtual TaskKind task_kind() const noexcept = 0;
  [[nodiscard]] virtual bool trained() const noexcept = 0;

  virtual Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) = 0;
  [[nodiscard]] virtual StatusOr<Series> infer(const FeatureBatch& batch, const InferOptions& options = {}) const = 0;
  [[nodiscard]] virtual StatusOr<Series> assess(const LabeledBatch& batch,
                                                const AssessOptions& options = {}) const = 0;

  [[nodiscard]] virtual StatusOr<std::vector<Series>> dump_state() const {
    return StatusOr<std::vector<Series>>(Status::not_implemented("dump_state() is not implemented for this estimator."));
  }
  virtual Status load_state(pluss::msharp::RangeView::Series state) {
    (void)state;
    return Status::not_implemented("load_state() is not implemented for this estimator.");
  }
};

class ClassifierBase : public EstimatorBase {
public:
  [[nodiscard]] TaskKind task_kind() const noexcept override { return TaskKind::Classification; }
  [[nodiscard]] virtual StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                                      const InferOptions& options = {}) const = 0;
};

class RegressorBase : public EstimatorBase {
public:
  [[nodiscard]] TaskKind task_kind() const noexcept override { return TaskKind::Regression; }
};

class TransformerBase {
public:
  virtual ~TransformerBase() = default;

  [[nodiscard]] virtual bool trained() const noexcept = 0;
  virtual Status learn(const FeatureBatch& batch, const TrainOptions& options = {}) = 0;
  [[nodiscard]] virtual StatusOr<Series> map(const FeatureBatch& batch, const InferOptions& options = {}) const = 0;
  [[nodiscard]] virtual StatusOr<Series> learn_map(const FeatureBatch& batch,
                                                   const TrainOptions& learn_options = {},
                                                   const InferOptions& infer_options = {}) = 0;

  // Supervised transforms (target / weight-of-evidence encoding) override these:
  // the pipeline hands the training target to `learn_supervised` when
  // `needs_target()` is true, and otherwise falls back to `learn`.
  [[nodiscard]] virtual bool needs_target() const noexcept { return false; }
  virtual Status learn_supervised(const FeatureBatch& batch,
                                  const Series& target,
                                  const TrainOptions& options = {}) {
    (void)target;
    return learn(batch, options);
  }

  [[nodiscard]] virtual StatusOr<std::vector<Series>> dump_state() const {
    return StatusOr<std::vector<Series>>(Status::not_implemented("dump_state() is not implemented for this transformer."));
  }
  virtual Status load_state(pluss::msharp::RangeView::Series state) {
    (void)state;
    return Status::not_implemented("load_state() is not implemented for this transformer.");
  }
};

} // namespace pluss::ai::nodej
