#include "pluss/ai/ice/source.h"

#include "pluss/tils.h"

#include <algorithm>
#include <numeric>
#include <random>

namespace pluss::ai::ice {

namespace {

[[nodiscard]] bool is_floating_dtype_(ScalarType dtype) {
  return dtype == ScalarType::Float16 || dtype == ScalarType::Float32 || dtype == ScalarType::Float64;
}

[[nodiscard]] Series maybe_cast_execution_dtype_(const Series& x, const SourceContext& ctx) {
  Series out = x.to(ctx.target_platform);
  const ScalarType dtype = ctx.target_dtype;
  if (is_floating_dtype_(out.scalar_type()) && out.scalar_type() != dtype) {
    out = out.to(dtype);
  }
  return out;
}

[[nodiscard]] data::Batch make_tensor_batch_(const Series& x, const std::optional<Series>& y) {
  data::Batch batch(data::Layout::CrossSection);
  batch.set(std::string(data::field_names::features), x);
  if (y.has_value()) {
    batch.set_target(*y);
  }
  return batch;
}

[[nodiscard]] Series index_from_rows_(const std::vector<std::int64_t>& rows,
                                      Platform platform) {
  const Config cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  return pluss::make_series(rows, {static_cast<std::int64_t>(rows.size())}, cfg).to(platform);
}

[[nodiscard]] data::Batch slice_batch_rows_(const data::Batch& batch,
                                            const std::vector<std::int64_t>& rows) {
  data::Batch out(batch.layout());
  for (const auto& field : batch.fields()) {
    const Series index = index_from_rows_(rows, field.values.platform());
    out.set(field.name, field.values.index_select(0, index), field.categoricals);
  }
  if (batch.target().defined()) {
    const Series target_index = index_from_rows_(rows, batch.target().platform());
    out.set_target(batch.target().index_select(0, target_index));
  }
  return out;
}

[[nodiscard]] std::vector<std::vector<std::int64_t>> planned_rows_(std::int64_t n,
                                                                   const BatchPlan& plan) {
  std::vector<std::int64_t> order(static_cast<std::size_t>(n));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  if (plan.shuffle && n > 1) {
    std::mt19937_64 rng(plan.seed);
    std::ranges::shuffle(order, rng);
  }

  const std::int64_t batch_size =
      (plan.batch_size <= 0 || plan.batch_size >= n) ? n : plan.batch_size;
  std::vector<std::vector<std::int64_t>> out{};
  out.reserve(static_cast<std::size_t>((n + batch_size - 1) / batch_size));
  for (std::int64_t start = 0; start < n; start += batch_size) {
    const std::int64_t end = std::min(n, start + batch_size);
    out.emplace_back(order.begin() + start, order.begin() + end);
  }
  return out;
}

[[nodiscard]] std::vector<data::Batch> shard_batches_(const data::Batch& batch,
                                                      const BatchPlan& plan) {
  PLUSS_CHECK(batch.size() > 0,
              "pluss::ai::ice::Source: cannot shard an empty batch.");
  const auto row_groups = planned_rows_(batch.size(), plan);
  std::vector<data::Batch> out{};
  out.reserve(row_groups.size());
  for (const auto& rows : row_groups) {
    out.push_back(slice_batch_rows_(batch, rows));
  }
  return out;
}

[[nodiscard]] BatchPlan single_batch_plan_() {
  return BatchPlan{
      .batch_size = 0,
      .shuffle = false,
      .seed = 0,
  };
}

} // namespace

void Source::before_transfer(const data::Batch&, const SourceContext&) const {}

void Source::after_transfer(Batch&, const SourceContext&) const {}

Batch Source::transfer_batch(const data::Batch& batch, const SourceContext& ctx) const {
  PLUSS_CALL_GUARD();
  before_transfer(batch, ctx);
  Batch out = do_transfer_batch(batch, ctx);
  after_transfer(out, ctx);
  return out;
}

TensorSource::TensorSource(Series x_train,
                           Series y_train,
                           std::optional<Series> x_valid,
                           std::optional<Series> y_valid,
                           BatchPlan learn_plan)
    : x_train_(std::move(x_train)),
      y_train_(std::move(y_train)),
      x_valid_(std::move(x_valid)),
      y_valid_(std::move(y_valid)),
      learn_plan_(std::move(learn_plan)) {}

TensorSource::TensorSource(Series x_infer, BatchPlan infer_plan)
    : x_infer_(std::move(x_infer)),
      infer_plan_(std::move(infer_plan)) {}

TensorSource::TensorSource(Series x_test, Series y_test)
    : x_test_(std::move(x_test)),
      y_test_(std::move(y_test)) {}

void TensorSource::prepare() {
  PLUSS_CALL_GUARD();
  if (prepared_) return;
  learn_batches_.clear();
  valid_batches_.clear();
  test_batches_.clear();
  infer_batches_.clear();

  if (x_train_.defined()) {
    learn_batches_ = shard_batches_(make_tensor_batch_(x_train_, y_train_), learn_plan_);
  }
  if (x_valid_.has_value()) {
    PLUSS_CHECK(y_valid_.has_value(),
                "pluss::ai::ice::TensorSource: x_valid was provided without y_valid.");
    valid_batches_ = shard_batches_(make_tensor_batch_(*x_valid_, y_valid_), single_batch_plan_());
  }
  if (x_test_.has_value()) {
    PLUSS_CHECK(y_test_.has_value(),
                "pluss::ai::ice::TensorSource: x_test was provided without y_test.");
    test_batches_ = shard_batches_(make_tensor_batch_(*x_test_, y_test_), single_batch_plan_());
  }
  if (x_infer_.has_value()) {
    infer_batches_ = shard_batches_(make_tensor_batch_(*x_infer_, std::nullopt), infer_plan_);
  }
  prepared_ = true;
}

Batch TensorSource::do_transfer_batch(const data::Batch& batch, const SourceContext& ctx) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(batch.has(data::field_names::features),
              "pluss::ai::ice::TensorSource: expected a 'features' field.");
  Batch out{};
  out.set_inputs(maybe_cast_execution_dtype_(batch.field(data::field_names::features), ctx));
  if (batch.target().defined()) {
    out.set_targets(maybe_cast_execution_dtype_(batch.target(), ctx));
  }
  return out;
}

DataSetSource::DataSetSource(data::DataSet train,
                             std::optional<data::DataSet> valid,
                             std::optional<data::DataSet> test,
                             std::optional<data::DataSet> infer,
                             BatchPlan learn_plan,
                             BatchPlan eval_plan)
    : train_(std::move(train)),
      valid_(std::move(valid)),
      test_(std::move(test)),
      infer_(std::move(infer)),
      learn_plan_(std::move(learn_plan)),
      eval_plan_(std::move(eval_plan)) {}

void DataSetSource::prepare() {
  PLUSS_CALL_GUARD();
  if (prepared_) return;
  learn_batches_.clear();
  valid_batches_.clear();
  test_batches_.clear();
  infer_batches_.clear();

  learn_batches_ = shard_batches_(train_.batch(), learn_plan_);
  if (valid_.has_value()) {
    valid_batches_ = shard_batches_(valid_->batch(), eval_plan_.batch_size == 0 ? single_batch_plan_() : eval_plan_);
  }
  if (test_.has_value()) {
    test_batches_ = shard_batches_(test_->batch(), eval_plan_.batch_size == 0 ? single_batch_plan_() : eval_plan_);
  }
  if (infer_.has_value()) {
    infer_batches_ = shard_batches_(infer_->batch(), eval_plan_);
  }
  prepared_ = true;
}

Batch DataSetSource::do_transfer_batch(const data::Batch& batch, const SourceContext& ctx) const {
  PLUSS_CALL_GUARD();
  Batch out{};
  for (const auto& field : batch.fields()) {
    Series values = maybe_cast_execution_dtype_(field.values, ctx);
    out.set(field.name, values);
    if (field.name == data::field_names::features) {
      out.set_inputs(values);
    }
  }
  if (batch.target().defined()) {
    Series target = maybe_cast_execution_dtype_(batch.target(), ctx);
    if (batch.layout() == data::Layout::CrossSection && target.sizes().size() == 1) {
      target = target.reshape({target.numel(), 1});
    }
    out.set_targets(std::move(target));
  }
  return out;
}

} // namespace pluss::ai::ice
