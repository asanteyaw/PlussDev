#include "pluss/ai/ice/model.h"

#include "pluss/ai/ice/coach.h"
#include "pluss/ai/ice/io.h"

#include "pluss/tils.h"
#include "table/DataFrame.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace pluss::ai::ice {

namespace {

// ice::io::save_parameters()/load_parameters() require the caller to pass an
// already-correctly-shaped Series to load into (a deliberate architecture-
// match guard for real trainable parameters, reused correctly by Model's own
// save()/load() for all_parameters_()). Standardization stats don't fit that
// contract -- their length (the feature count) isn't known until the file
// itself is read -- so this is a small, self-contained sidecar format
// instead of forcing them through the parameter-list one.
void save_standardize_sidecar_(const std::string& path, const Series& mean, const Series& std) {
  const auto mean_vec = mean.reshape({mean.numel()}).to(pluss::pCPU, ScalarType::Float64).host<double>();
  const auto std_vec = std.reshape({std.numel()}).to(pluss::pCPU, ScalarType::Float64).host<double>();
  std::ofstream out{path};
  PLUSS_CHECK(out.good(), "pluss::ai::ice::Model::save: failed to open '" + path + "'.");
  out << "pluss_ice_standardize\n";
  out << "count=" << mean_vec.size() << "\n";
  for (const double v : mean_vec) out << v << " ";
  out << "\n";
  for (const double v : std_vec) out << v << " ";
  out << "\n";
  PLUSS_CHECK(out.good(), "pluss::ai::ice::Model::save: failed while writing '" + path + "'.");
}

void load_standardize_sidecar_(const std::string& path, Series& mean, Series& std) {
  std::ifstream in{path};
  PLUSS_CHECK(in.good(), "pluss::ai::ice::Model::load: failed to open '" + path + "'.");
  std::string line;
  PLUSS_CHECK(std::getline(in, line) && line == "pluss_ice_standardize",
             "pluss::ai::ice::Model::load: invalid standardization sidecar header in '" + path + "'.");
  PLUSS_CHECK(std::getline(in, line) && line.rfind("count=", 0) == 0,
             "pluss::ai::ice::Model::load: malformed standardization sidecar in '" + path + "'.");
  const auto count = std::stoll(line.substr(6));

  std::vector<double> mean_vec(static_cast<std::size_t>(count));
  std::vector<double> std_vec(static_cast<std::size_t>(count));
  for (auto& v : mean_vec) PLUSS_CHECK(static_cast<bool>(in >> v), "pluss::ai::ice::Model::load: truncated standardization sidecar in '" + path + "'.");
  for (auto& v : std_vec) PLUSS_CHECK(static_cast<bool>(in >> v), "pluss::ai::ice::Model::load: truncated standardization sidecar in '" + path + "'.");

  const Config cfg = Config{}.with_dtype(mean.scalar_type()).with_platform(mean.platform());
  mean = pluss::make_series(mean_vec, {count}, cfg).to(cfg.platform, cfg.dtype);
  std = pluss::make_series(std_vec, {count}, cfg).to(cfg.platform, cfg.dtype);
}

} // namespace

Series Model::forward(const Batch& batch) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(batch.has_inputs(),
              "pluss::ai::ice::Model::forward(batch): batch.inputs must be defined.");
  return forward(batch.inputs());
}

Series Model::compute_loss(const Series& predictions, const Series& y, LossKind kind) const {
  switch (kind) {
    case LossKind::MSE:
      return pluss::nn::losses::MSE(predictions, y);
    case LossKind::BCEWithLogits:
      return pluss::nn::losses::BCEWithLogits(predictions, y);
    case LossKind::CrossEntropy:
      return pluss::nn::losses::CrossEntropy(predictions, y);
  }
  PLUSS_FAIL("pluss::ai::ice::Model::compute_loss: unknown LossKind.");
}

Series Model::compute_loss(const Series& predictions,
                           const Series& y,
                           LossKind kind,
                           double label_smoothing) const {
  if (kind == LossKind::CrossEntropy && label_smoothing != 0.0) {
    return pluss::nn::losses::CrossEntropy(predictions, y, {.label_smoothing = label_smoothing});
  }
  return compute_loss(predictions, y, kind);
}

Series Model::apply_objective_regularization_(const Series& base_loss, const StepContext& ctx) const {
  PLUSS_CALL_GUARD();
  if (ctx.recipe == nullptr) {
    return base_loss;
  }

  Series out = base_loss;
  const auto& regularization = ctx.recipe->objective_regularization;
  const auto parameters = all_parameters_();
  if (regularization.l1_penalty != 0.0) {
    Series l1 = Series::full({}, 0.0, base_loss.scalar_type()).to(base_loss.platform());
    for (const Series* parameter : parameters) {
      if (parameter == nullptr || !parameter->defined()) continue;
      l1 = l1.add(parameter->abs().sum());
    }
    out = out.add(l1.mul(regularization.l1_penalty));
  }
  if (regularization.l2_penalty != 0.0) {
    Series l2 = Series::full({}, 0.0, base_loss.scalar_type()).to(base_loss.platform());
    for (const Series* parameter : parameters) {
      if (parameter == nullptr || !parameter->defined()) continue;
      l2 = l2.add(parameter->mul(*parameter).sum());
    }
    out = out.add(l2.mul(regularization.l2_penalty));
  }
  return out;
}

Series Model::compute_loss(const Series& predictions,
                           const Batch& batch,
                           const StepContext& ctx) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(batch.has_targets(),
              "pluss::ai::ice::Model::compute_loss(batch): batch.targets must be defined.");
  PLUSS_CHECK(ctx.loss_kind.has_value(),
              "pluss::ai::ice::Model::compute_loss(batch): StepContext.loss_kind must be set.");
  const double label_smoothing =
      ctx.recipe != nullptr ? ctx.recipe->objective_regularization.label_smoothing : 0.0;
  return apply_objective_regularization_(
      compute_loss(predictions, batch.targets(), *ctx.loss_kind, label_smoothing), ctx);
}

StepResult Model::learn_step(const Batch& batch, const StepContext& ctx) const {
  PLUSS_CALL_GUARD();
  const Series predictions = forward(batch);
  return StepResult{
      .predictions = predictions,
      .loss = compute_loss(predictions, batch, ctx),
  };
}

StepResult Model::valid_step(const Batch& batch, const StepContext& ctx) const {
  PLUSS_CALL_GUARD();
  const Series predictions = forward(batch);
  return StepResult{
      .predictions = predictions,
      .loss = compute_loss(predictions, batch, ctx),
  };
}

StepResult Model::test_step(const Batch& batch, const StepContext& ctx) const {
  PLUSS_CALL_GUARD();
  const Series predictions = forward(batch);
  return StepResult{
      .predictions = predictions,
      .loss = compute_loss(predictions, batch, ctx),
  };
}

StepResult Model::infer_step(const Batch& batch, const StepContext& ctx) const {
  PLUSS_CALL_GUARD();
  static_cast<void>(ctx);
  return StepResult{
      .predictions = forward(batch),
      .loss = pluss::series::undefined(),
  };
}

void Model::register_parameter(Series& param) {
  owned_parameters_.push_back(&param);
}

void Model::register_module(pluss::nn::Module& module) {
  module.train(training_);
  owned_modules_.push_back(&module);
}

void Model::train(bool enabled) noexcept {
  training_ = enabled;
  for (auto* module : owned_modules_) {
    module->train(enabled);
  }
}

Model& Model::to(Platform platform) {
  PLUSS_CALL_GUARD();
  // Raw registered Series aren't Modules, so they don't get a recursive
  // Module::to() sweep for free — move them one at a time. Modules (e.g.
  // ice::Outline's pluss::nn::Schedule) already move every parameter and
  // buffer in their own subtree via a single .to(platform) call.
  for (auto* param : owned_parameters_) {
    *param = param->to(platform);
  }
  for (auto* module : owned_modules_) {
    module->to(platform);
  }
  if (standardized_) {
    standardize_mean_ = standardize_mean_.to(platform);
    standardize_std_ = standardize_std_.to(platform);
  }
  train(training_);
  return *this;
}

Model& Model::to(ScalarType dtype) {
  PLUSS_CALL_GUARD();
  for (auto* param : owned_parameters_) {
    *param = param->to(dtype);
  }
  for (auto* module : owned_modules_) {
    module->to(dtype);
  }
  if (standardized_) {
    standardize_mean_ = standardize_mean_.to(dtype);
    standardize_std_ = standardize_std_.to(dtype);
  }
  train(training_);
  return *this;
}

Model& Model::to(Platform platform, ScalarType dtype) {
  PLUSS_CALL_GUARD();
  for (auto* param : owned_parameters_) {
    *param = param->to(platform, dtype);
  }
  for (auto* module : owned_modules_) {
    module->to(platform, dtype);
  }
  if (standardized_) {
    standardize_mean_ = standardize_mean_.to(platform, dtype);
    standardize_std_ = standardize_std_.to(platform, dtype);
  }
  train(training_);
  return *this;
}

std::vector<Series*> Model::all_parameters_() const {
  std::vector<Series*> out{owned_parameters_};
  for (auto* module : owned_modules_) {
    const auto module_params = module->parameters();
    out.insert(out.end(), module_params.begin(), module_params.end());
  }
  return out;
}

Series Model::dataframe_matrix_(const pluss::table::DataFrame& df, std::string_view exclude) const {
  PLUSS_CALL_GUARD();
  const auto& all_columns = df.features();
  std::vector<std::string_view> feature_names{};
  feature_names.reserve(all_columns.size());
  for (const auto& name : all_columns) {
    if (name != exclude) feature_names.push_back(name);
  }
  PLUSS_CHECK(!feature_names.empty(),
             "pluss::ai::ice::Model: DataFrame has no usable feature columns" +
                 std::string(exclude.empty() ? "." : " besides the target."));
  return df.features(pluss::msharp::RangeView::Of<std::string_view>(feature_names))
             .matrix()
             .as_series()
             .to(pluss::pCPU, ScalarType::Float32);
}

Series Model::fit_standardize_(const Series& x) {
  PLUSS_CALL_GUARD();
  // Entirely device-resident (CPU/CUDA/MPS, whichever `x` already lives on)
  // — no host round-trip, no manual per-element loop. Reducing to
  // per-feature scalars first means the *shape* of the work is a batched
  // subtract/multiply/mean over all N rows at once, not a loop over them.
  standardize_mean_ = x.mean(0);                                    // [F]
  const Series centered = x.sub(standardize_mean_);                  // [N, F] - [F] broadcasts
  const Series col_variance = centered.mul(centered).mean(0);        // [F]
  standardize_std_ = col_variance.clamp_min(1e-12).sqrt();
  standardized_ = true;
  return centered.div(standardize_std_);
}

Series Model::apply_standardize_(const Series& x) const {
  PLUSS_CALL_GUARD();
  if (!standardized_) return x;
  // Reconciles platform against `x` rather than assuming either side is
  // already in the right place — cheap, since the stats are [F]-shaped.
  return x.sub(standardize_mean_.to(x.platform())).div(standardize_std_.to(x.platform()));
}

void Model::learn(const Series& x_train, const Series& y_train, const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  Coach(options).learn(*this, x_train, y_train);
}

void Model::learn(Source& source, const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  Coach(options).learn(*this, source);
}

void Model::learn(const Series& x_train, const Series& y_train,
                  const Series& x_val, const Series& y_val,
                  const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  Coach(options).learn(*this, x_train, y_train, x_val, y_val);
}

void Model::learn(const pluss::table::DataFrame& df, std::string_view target, const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  Coach(options).learn(*this, df, target);
}

Series Model::infer(const Series& x, std::int64_t chunk_size) const {
  PLUSS_CALL_GUARD();
  return Coach{}.infer(*this, x, chunk_size);
}

Series Model::infer(Source& source) const {
  PLUSS_CALL_GUARD();
  return Coach{}.infer(*this, source);
}

Series Model::infer(const pluss::table::DataFrame& df, std::string_view target, std::int64_t chunk_size) const {
  PLUSS_CALL_GUARD();
  return Coach{}.infer(*this, df, target, chunk_size);
}

AssessReport Model::assess(const Series& x, const Series& y, LossKind loss_kind) const {
  PLUSS_CALL_GUARD();
  return Coach{}.assess(*this, x, y, loss_kind);
}

AssessReport Model::assess(Source& source, LossKind loss_kind) const {
  PLUSS_CALL_GUARD();
  return Coach{}.assess(*this, source, loss_kind);
}

AssessReport Model::assess(const pluss::table::DataFrame& df, std::string_view target, LossKind loss) const {
  PLUSS_CALL_GUARD();
  return Coach{}.assess(*this, df, target, loss);
}

void Model::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  io::save_parameters(filepath, all_parameters_());
  if (standardized_) {
    save_standardize_sidecar_(std::string(filepath) + ".standardize", standardize_mean_, standardize_std_);
  }
}

void Model::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  io::load_parameters(filepath, all_parameters_());
  const std::string sidecar_path = std::string(filepath) + ".standardize";
  if (std::filesystem::exists(sidecar_path)) {
    load_standardize_sidecar_(sidecar_path, standardize_mean_, standardize_std_);
    standardized_ = true;
  } else {
    standardized_ = false;
  }
  trained_ = true;
}

} // namespace pluss::ai::ice
