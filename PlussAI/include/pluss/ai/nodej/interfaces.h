#pragma once

#include "pluss/ai/nodej/dataset.h"

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
};

} // namespace pluss::ai::nodej
