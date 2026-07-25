#include "pluss/ai/ice/model.h"

#include "pluss/ai/ice/io.h"
#include "pluss/ai/ice/no_grad.h"
#include "pluss/ai/ice/optim.h"

#include "pluss/tils.h"
#include "table/DataFrame.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
  const auto mean_vec = mean.reshape({mean.numel()}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  const auto std_vec = std.reshape({std.numel()}).to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
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

void Model::register_parameter(Series& param) {
  owned_parameters_.push_back(&param);
}

void Model::register_module(pluss::nn::Module& module) {
  owned_modules_.push_back(&module);
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

void Model::train_loop_(const Series& x_train, const Series& y_train,
                        const Series* x_val, const Series* y_val,
                        const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(options.max_epochs > 0 && options.batch_size > 0,
             "pluss::ai::ice::Model::learn: max_epochs and batch_size must be positive.");

  const auto parameters = all_parameters_();
  PLUSS_CHECK(!parameters.empty(),
             "pluss::ai::ice::Model::learn: model has no registered parameters "
             "(call register_parameter/register_module in the constructor).");

  const Platform target_platform = resolve_platform(options.platform);
  to(target_platform);

  auto optimizer = make_optimizer(parameters, options);

  pluss::loader::DataLoader loader(
      {x_train, y_train},
      pluss::loader::DataLoaderOptions{
          .batch_size = options.batch_size,
          .shuffle = options.shuffle,
          .seed = options.seed,
          .target_platform = target_platform,
      });

  // Moved once, up front, rather than per-epoch: the val set is small
  // relative to the training set and reused across every epoch, so this is
  // a single amortized transfer rather than a repeated one.
  const Series x_val_on_device = x_val != nullptr ? x_val->to(target_platform) : Series{};
  const Series y_val_on_device = y_val != nullptr ? y_val->to(target_platform) : Series{};

  double best_val_loss = std::numeric_limits<double>::infinity();
  std::int64_t epochs_without_improvement = 0;

  try {
    for (std::int64_t epoch = 0; epoch < options.max_epochs; ++epoch) {
      std::cerr << "[DEBUG train_loop_] epoch " << epoch << " start\n" << std::flush;
      for (const auto batch : loader) {
        const Series predictions = forward(batch.x());
        const Series loss = compute_loss(predictions, batch.y(), options.loss);
        optimizer->clear();
        loss.reverse();
        optimizer->step();
      }
      std::cerr << "[DEBUG train_loop_] epoch " << epoch << " done\n" << std::flush;

      if (options.early_stopping && x_val != nullptr && y_val != nullptr) {
        const Series val_predictions = forward(x_val_on_device);
        const Series val_loss = compute_loss(val_predictions, y_val_on_device, options.loss);
        const double val_loss_value = val_loss.item<double>();
        if (val_loss_value < best_val_loss - options.min_delta) {
          best_val_loss = val_loss_value;
          epochs_without_improvement = 0;
        } else if (++epochs_without_improvement >= options.patience) {
          break;
        }
      }
    }
  } catch (const std::exception& e) {
    PLUSS_FAIL(std::string("pluss::ai::ice::Model::learn: training became numerically unstable: ") +
              e.what());
  }

  trained_ = true;
}

void Model::learn(const Series& x_train, const Series& y_train, const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  if (options.val_ratio > 0.0) {
    const auto split = pluss::loader::split_fields(
        {x_train, y_train},
        pluss::loader::DataSplitOptions{
            .train_ratio = 1.0 - options.val_ratio,
            .val_ratio = options.val_ratio,
            .shuffle = options.shuffle,
            .seed = options.seed,
        });
    train_loop_(split.train_fields[0], split.train_fields[1],
               &split.val_fields[0], &split.val_fields[1], options);
  } else {
    train_loop_(x_train, y_train, nullptr, nullptr, options);
  }
}

void Model::learn(const Series& x_train, const Series& y_train,
                  const Series& x_val, const Series& y_val,
                  const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  train_loop_(x_train, y_train, &x_val, &y_val, options);
}

void Model::learn(const pluss::table::DataFrame& df, std::string_view target, const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  Series x = dataframe_matrix_(df, target);
  Series y = df.features(target).as_series().to(pluss::pCPU, ScalarType::Float32);
  if (y.sizes().size() == 1) {
    y = y.reshape({y.numel(), 1});
  }

  if (options.standardize) {
    x = fit_standardize_(x);
  }
  learn(x, y, options);
}

Series Model::infer(const Series& x, std::int64_t chunk_size) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(trained_, "pluss::ai::ice::Model::infer: model has not been trained.");

  const auto parameters = all_parameters_();
  const NoGradGuard guard(parameters);
  // Matches `x` to wherever the model's parameters currently live, rather
  // than requiring the caller to track that themselves — a model trained
  // with TrainOptions::platform = GPU stays on GPU after learn() returns.
  const Series x_on_device = x.to(parameters.front()->platform());
  const auto n = x_on_device.sizes()[0];
  if (chunk_size <= 0 || n <= chunk_size) {
    return forward(x_on_device);
  }
  std::vector<Series> chunks{};
  chunks.reserve(static_cast<std::size_t>((n + chunk_size - 1) / chunk_size));
  for (std::int64_t start = 0; start < n; start += chunk_size) {
    const auto length = std::min(chunk_size, n - start);
    chunks.push_back(forward(x_on_device.narrow_copy(0, start, length)));
  }
  return pluss::unite(chunks, 0);
}

Series Model::infer(const pluss::table::DataFrame& df, std::string_view target, std::int64_t chunk_size) const {
  PLUSS_CALL_GUARD();
  const Series x = apply_standardize_(dataframe_matrix_(df, target));
  return infer(x, chunk_size);
}

AssessReport Model::assess(const Series& x, const Series& y, LossKind loss_kind) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(trained_, "pluss::ai::ice::Model::assess: model has not been trained.");

  const auto parameters = all_parameters_();
  const NoGradGuard guard(parameters);
  const Platform model_platform = parameters.front()->platform();
  const Series y_on_device = y.to(model_platform);
  const Series predictions = forward(x.to(model_platform));
  const Series loss = compute_loss(predictions, y_on_device, loss_kind);

  AssessReport report{};
  report.loss = loss.item<double>();

  switch (loss_kind) {
    case LossKind::MSE: {
      const Series diff = predictions.sub(y_on_device);
      const Series mse = diff.mul(diff).mean();
      report.metric = mse.sqrt().item<double>();
      report.metric_name = "rmse";
      break;
    }
    case LossKind::BCEWithLogits: {
      const Series target = y_on_device.to(ScalarType::Float32);
      const Series predicted = predictions.sigmoid().ge(0.5).to(ScalarType::Float32);
      const Series correct = predicted.eq(target).to(ScalarType::Float32);
      report.metric = correct.mean().item<double>();
      report.metric_name = "accuracy";
      break;
    }
    case LossKind::CrossEntropy: {
      const Series target = y_on_device.to(ScalarType::Float32);
      const Series predicted = predictions.argmax(predictions.sizes().size() - 1).to(ScalarType::Float32);
      const Series correct = predicted.eq(target).to(ScalarType::Float32);
      report.metric = correct.mean().item<double>();
      report.metric_name = "accuracy";
      break;
    }
  }
  return report;
}

AssessReport Model::assess(const pluss::table::DataFrame& df, std::string_view target, LossKind loss) const {
  PLUSS_CALL_GUARD();
  const Series x = apply_standardize_(dataframe_matrix_(df, target));
  Series y = df.features(target).as_series().to(pluss::pCPU, ScalarType::Float32);
  if (y.sizes().size() == 1) {
    y = y.reshape({y.numel(), 1});
  }
  return assess(x, y, loss);
}

void Model::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(trained_, "pluss::ai::ice::Model::save: model has not been trained.");
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
