#pragma once

#include "pluss/ai/ice/data.h"
#include "pluss/ai/ice/step.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace pluss::ai::ice {

struct BatchPlan {
  std::int64_t batch_size = 0;
  bool shuffle = false;
  std::uint64_t seed = 0;
};

struct SourceContext {
  const TrainOptions* recipe = nullptr;
  StepStage stage = StepStage::Infer;
  Platform target_platform = pluss::pCPU;
  ScalarType target_dtype = ScalarType::Float32;
};

class Source {
public:
  virtual ~Source() = default;

  virtual void prepare() = 0;
  [[nodiscard]] virtual const std::vector<data::Batch>& learn_batches() const = 0;
  [[nodiscard]] virtual const std::vector<data::Batch>& valid_batches() const = 0;
  [[nodiscard]] virtual const std::vector<data::Batch>& test_batches() const = 0;
  [[nodiscard]] virtual const std::vector<data::Batch>& infer_batches() const = 0;

  [[nodiscard]] Batch transfer_batch(const data::Batch& batch, const SourceContext& ctx) const;

protected:
  virtual void before_transfer(const data::Batch& batch, const SourceContext& ctx) const;
  [[nodiscard]] virtual Batch do_transfer_batch(const data::Batch& batch, const SourceContext& ctx) const = 0;
  virtual void after_transfer(Batch& batch, const SourceContext& ctx) const;
};

class TensorSource final : public Source {
public:
  TensorSource(Series x_train,
               Series y_train,
               std::optional<Series> x_valid,
               std::optional<Series> y_valid,
               BatchPlan learn_plan);
  explicit TensorSource(Series x_infer, BatchPlan infer_plan = {});
  TensorSource(Series x_test, Series y_test);

  void prepare() override;
  [[nodiscard]] const std::vector<data::Batch>& learn_batches() const override { return learn_batches_; }
  [[nodiscard]] const std::vector<data::Batch>& valid_batches() const override { return valid_batches_; }
  [[nodiscard]] const std::vector<data::Batch>& test_batches() const override { return test_batches_; }
  [[nodiscard]] const std::vector<data::Batch>& infer_batches() const override { return infer_batches_; }

protected:
  [[nodiscard]] Batch do_transfer_batch(const data::Batch& batch, const SourceContext& ctx) const override;

private:
  Series x_train_ = Series::undefined();
  Series y_train_ = Series::undefined();
  std::optional<Series> x_valid_ = std::nullopt;
  std::optional<Series> y_valid_ = std::nullopt;
  std::optional<Series> x_test_ = std::nullopt;
  std::optional<Series> y_test_ = std::nullopt;
  std::optional<Series> x_infer_ = std::nullopt;
  BatchPlan learn_plan_{};
  BatchPlan infer_plan_{};
  bool prepared_ = false;
  std::vector<data::Batch> learn_batches_{};
  std::vector<data::Batch> valid_batches_{};
  std::vector<data::Batch> test_batches_{};
  std::vector<data::Batch> infer_batches_{};
};

class DataSetSource final : public Source {
public:
  DataSetSource(data::DataSet train,
                std::optional<data::DataSet> valid = std::nullopt,
                std::optional<data::DataSet> test = std::nullopt,
                std::optional<data::DataSet> infer = std::nullopt,
                BatchPlan learn_plan = {},
                BatchPlan eval_plan = {});

  void prepare() override;
  [[nodiscard]] const std::vector<data::Batch>& learn_batches() const override { return learn_batches_; }
  [[nodiscard]] const std::vector<data::Batch>& valid_batches() const override { return valid_batches_; }
  [[nodiscard]] const std::vector<data::Batch>& test_batches() const override { return test_batches_; }
  [[nodiscard]] const std::vector<data::Batch>& infer_batches() const override { return infer_batches_; }

protected:
  [[nodiscard]] Batch do_transfer_batch(const data::Batch& batch, const SourceContext& ctx) const override;

private:
  data::DataSet train_;
  std::optional<data::DataSet> valid_;
  std::optional<data::DataSet> test_;
  std::optional<data::DataSet> infer_;
  BatchPlan learn_plan_{};
  BatchPlan eval_plan_{};
  bool prepared_ = false;
  std::vector<data::Batch> learn_batches_{};
  std::vector<data::Batch> valid_batches_{};
  std::vector<data::Batch> test_batches_{};
  std::vector<data::Batch> infer_batches_{};
};

} // namespace pluss::ai::ice
