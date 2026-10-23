#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config work_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Series as_matrix_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0], 1});
  }
  PLUSS_CHECK(sizes.size() == 2, std::string("subsp::stats::ts::") + name + ": expected 1D or 2D Series");
  return x;
}

Series as_vector_(const Series& x, const char* name) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + name + ": expected defined Series");
  const auto sizes = x.sizes();
  PLUSS_CHECK(!sizes.empty(), std::string("subsp::stats::ts::") + name + ": expected non-scalar Series");
  if (sizes.size() == 1) {
    return x.reshape({sizes[0]});
  }
  PLUSS_CHECK(sizes.size() == 2 && sizes[1] == 1,
              std::string("subsp::stats::ts::") + name + ": expected vector or Nx1 matrix");
  return x.reshape({sizes[0]});
}

Series i64_range_(std::int64_t start, std::int64_t end_exclusive, Config cfg) {
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return pluss::series::arange(start, end_exclusive, idx_cfg);
}

Series active_columns_index_for_horizon_(const Series& mask, std::int64_t hidx, Config cfg) {
  const auto out_cfg = work_cfg_(cfg);
  const Series row = mask.select(0, hidx).to(out_cfg.platform, ScalarType::Bool).reshape({mask.sizes()[1]});
  const Series nz = row.nonzero();
  if (!nz.defined() || nz.numel() == 0) {
    return pluss::series::empty({0}, Config{}.with_dtype(ScalarType::Int64).with_platform(out_cfg.platform));
  }
  return nz.select(1, 0).reshape({nz.sizes()[0]}).to(out_cfg.platform, ScalarType::Int64);
}

Series select_rows_(const Series& x, std::int64_t start, std::int64_t len, Config cfg) {
  PLUSS_CHECK(start >= 0, "subsp::stats::ts::DirectForecaster: row start must be non-negative");
  PLUSS_CHECK(len > 0, "subsp::stats::ts::DirectForecaster: row length must be positive");
  PLUSS_CHECK(start + len <= x.sizes()[0], "subsp::stats::ts::DirectForecaster: row slice exceeds bounds");
  return x.index_select(0, i64_range_(start, start + len, cfg));
}

Series select_columns_(const Series& x, const Series& col_idx, Config cfg) {
  PLUSS_CHECK(col_idx.defined() && col_idx.numel() > 0,
              "subsp::stats::ts::DirectForecaster: expected non-empty active feature set");
  const auto idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  return x.index_select(1, col_idx.to(idx_cfg.platform, idx_cfg.dtype));
}

Series prepend_intercept_(const Series& x, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  const Series intercept = pluss::series::ones({x.sizes()[0], 1}, out_cfg);
  return intercept.unite({x.to(out_cfg.platform, out_cfg.dtype)}, 1);
}

Series fit_beta_(const Series& x, const Series& y, const DirectForecasterOptions& opts, Config cfg) {
  const Series x64 = x.to(cfg.platform, ScalarType::Float64);
  const Series y64 = y.to(cfg.platform, ScalarType::Float64).reshape({y.numel(), 1});
  Series x_aug = x64;
  Series y_aug = y64;
  if (opts.learner == DirectLearnerKind::Ridge) {
    PLUSS_CHECK(opts.ridge_lambda >= 0.0, "subsp::stats::ts::DirectForecaster: ridge_lambda must be >= 0");
    if (opts.ridge_lambda > 0.0) {
      Series penalty = pluss::series::eye(x64.sizes()[1], x64.sizes()[1], cfg).mul(std::sqrt(opts.ridge_lambda));
      if (opts.fit_intercept) {
        const Series idx = pluss::make_series(std::vector<std::int64_t>{0},
                                              {1},
                                              Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform));
        penalty = pluss::index_put(penalty, std::vector<Series>{idx, idx}, pluss::series::zeros({1}, cfg), false);
      }
      x_aug = x64.unite({penalty}, 0);
      y_aug = y64.unite({pluss::series::zeros({x64.sizes()[1], 1}, cfg)}, 0);
    }
  }

  Series beta = pluss::series::undefined();
  const std::array<double, 5> scales{1.0, 10.0, 100.0, 1000.0, 1.0e6};
  const double base_rcond = std::max(1e-12, opts.pinv_rcond);
  for (double scale : scales) {
    beta = pluss::subsp::math::linalg::pinverse(x_aug, base_rcond * scale).matmul(y_aug).reshape({x64.sizes()[1]});
    if (beta.isfinite().all().item<double>() == 1.0) {
      break;
    }
  }
  PLUSS_CHECK(beta.isfinite().all().item<double>() == 1.0,
              "subsp::stats::ts::DirectForecaster: failed to produce finite coefficients");
  return beta;
}

// Rows (of an n x p matrix) in which every entry is finite, as an n-vector of 0/1 in work precision.
Series finite_rows_(const Series& x, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  const double p = static_cast<double>(x.sizes()[1]);
  const Series count = x.isfinite().to(out_cfg.platform, ScalarType::Float64).sum(1, false).reshape({x.sizes()[0]});
  return count.sub(p).abs().lt(0.5).to(out_cfg.platform, ScalarType::Float64);
}

// Indices of the nonzero entries of a 0/1 vector.
Series true_indices_(const Series& flags, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  const Series nz = flags.gt(0.5).nonzero();
  if (!nz.defined() || nz.numel() == 0) {
    return pluss::series::empty({0}, Config{}.with_dtype(ScalarType::Int64).with_platform(out_cfg.platform));
  }
  return nz.select(1, 0).reshape({nz.sizes()[0]}).to(out_cfg.platform, ScalarType::Int64);
}

// Predictions are NaN in rows whose features are not all available (the first rows, before enough history exists).
Series blank_unavailable_(const Series& pred, const Series& valid_rows, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  const std::int64_t rows = pred.sizes()[0];
  const Series keep = valid_rows.reshape({rows, 1}).gt(0.5);
  return keep.where(pred, pluss::series::full({rows, pred.sizes()[1]}, std::numeric_limits<double>::quiet_NaN(), out_cfg));
}

Series concat_columns_(const std::vector<Series>& columns, std::int64_t rows, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  if (columns.empty()) {
    return pluss::series::empty({rows, 0}, out_cfg);
  }
  if (columns.size() == 1) {
    return columns.front();
  }
  return columns.front().unite(std::vector<Series>(columns.begin() + 1, columns.end()), 1);
}

std::int64_t observed_count_(const Series& x, const Series& y, const LagPlan& plan) {
  if (y.defined()) {
    return as_vector_(y, "DirectForecaster").numel();
  }
  PLUSS_CHECK(plan.response_lags.empty(),
              "subsp::stats::ts::DirectForecaster: response series is required when response_lags is non-empty");
  return as_matrix_(x, "DirectForecaster").sizes()[0];
}

bool leading_requires_future_(const LagPlan& plan) {
  return std::any_of(plan.leading_unique_lags.begin(), plan.leading_unique_lags.end(), [](const auto& lags) {
    return std::any_of(lags.begin(), lags.end(), [](const std::int64_t lag) { return lag < 0; });
  });
}

const DirectStepModel& step_for_horizon_(const std::vector<DirectStepModel>& steps, std::int64_t horizon_step) {
  const auto it = std::find_if(steps.begin(), steps.end(), [horizon_step](const auto& s) { return s.horizon == horizon_step; });
  PLUSS_CHECK(it != steps.end(), "subsp::stats::ts::DirectForecaster: requested horizon step is not in model");
  return *it;
}

Series step_active_columns_index_(const DirectStepModel& step_model, Config cfg) {
  const auto idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  if (step_model.active_columns_index.defined() && step_model.active_columns_index.numel() > 0) {
    return step_model.active_columns_index.to(idx_cfg.platform, idx_cfg.dtype);
  }
  PLUSS_CHECK(!step_model.active_columns.empty(),
              "subsp::stats::ts::DirectForecaster: expected non-empty active feature set");
  return pluss::make_series(step_model.active_columns,
                            {static_cast<std::int64_t>(step_model.active_columns.size())},
                            idx_cfg);
}

} // namespace

DirectForecaster::DirectForecaster(LagPlan lag_plan, DirectForecasterOptions options, std::vector<DirectStepModel> steps)
    : lag_plan_(std::move(lag_plan)),
      options_(std::move(options)),
      steps_(std::move(steps)) {}

DirectForecaster DirectForecaster::fit(const Series& x, const Series& y, const DirectForecasterOptions& options, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectForecaster::fit").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  PLUSS_CHECK(nobs > 0, "subsp::stats::ts::DirectForecaster::fit: expected positive observation count");

  const std::int64_t num_predictors = x.defined() ? as_matrix_(x, "DirectForecaster::fit").sizes()[1] : 0;
  const LagPlan plan = build_lag_plan(num_predictors, options.lag);
  const Series design = build_lagged_predictors(x, yv, plan, out_cfg);
  const Series mask = plan.prepared_predictors_per_horizon.to(out_cfg.platform, ScalarType::Bool);

  std::vector<DirectStepModel> steps;
  steps.reserve(plan.horizon.size());
  for (std::size_t i = 0; i < plan.horizon.size(); ++i) {
    const std::int64_t h = plan.horizon[i];
    const std::int64_t row_start = plan.max_lead - (h - 1);
    Series x_step = select_rows_(design, row_start, nobs, out_cfg);
    const Series active_cols_idx = active_columns_index_for_horizon_(mask, static_cast<std::int64_t>(i), out_cfg);
    x_step = select_columns_(x_step, active_cols_idx, out_cfg);
    // Only rows with a complete feature vector and an observed response take part in the fit.
    const Series usable = finite_rows_(x_step, out_cfg).mul(yv.isfinite().to(out_cfg.platform, ScalarType::Float64));
    const Series rows = true_indices_(usable, out_cfg);
    PLUSS_CHECK(rows.numel() > 0, "subsp::stats::ts::DirectForecaster::fit: no observation has a complete set of lagged predictors");
    x_step = x_step.index_select(0, rows);
    if (options.fit_intercept) {
      x_step = prepend_intercept_(x_step, out_cfg);
    }
    steps.push_back(DirectStepModel{
        .horizon = h,
        .active_columns = {},
        .active_columns_index = active_cols_idx,
        .beta = fit_beta_(x_step, yv.index_select(0, rows), options, out_cfg),
    });
  }

  return DirectForecaster(plan, options, std::move(steps));
}

Series DirectForecaster::predict(const Series& x, const Series& y, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::predict: model has no fitted steps");
  const Config out_cfg = work_cfg_(cfg);
  const std::int64_t nobs = observed_count_(x, y, lag_plan_);
  const Series yv = y.defined() ? as_vector_(y, "DirectForecaster::predict").to(out_cfg.platform, out_cfg.dtype)
                                : pluss::series::undefined();

  const Series design = build_lagged_predictors(x, yv, lag_plan_, out_cfg);
  std::vector<Series> preds;
  preds.reserve(steps_.size());
  for (const auto& step_model : steps_) {
    const std::int64_t row_start = lag_plan_.max_lead - (step_model.horizon - 1);
    Series x_step = select_rows_(design, row_start, nobs, out_cfg);
    x_step = select_columns_(x_step, step_active_columns_index_(step_model, out_cfg), out_cfg);
    const Series valid = finite_rows_(x_step, out_cfg);
    x_step = x_step.nan_to_num(0.0, 0.0, 0.0);
    if (options_.fit_intercept) {
      x_step = prepend_intercept_(x_step, out_cfg);
    }
    preds.push_back(blank_unavailable_(x_step.matmul(step_model.beta.reshape({step_model.beta.numel(), 1})), valid, out_cfg));
  }
  return concat_columns_(preds, nobs, out_cfg);
}

Series DirectForecaster::prepared_predictors(const Series& x, const Series& y, std::int64_t horizon_step, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::prepared_predictors: model has no fitted steps");
  const auto out_cfg = work_cfg_(cfg);
  const auto& step_model = step_for_horizon_(steps_, horizon_step);
  const std::int64_t nobs = observed_count_(x, y, lag_plan_);
  const Series yv = y.defined() ? as_vector_(y, "DirectForecaster::prepared_predictors").to(out_cfg.platform, out_cfg.dtype)
                                : pluss::series::undefined();
  const Series design = build_lagged_predictors(x, yv, lag_plan_, out_cfg);
  const std::int64_t row_start = lag_plan_.max_lead - (step_model.horizon - 1);
  Series x_step = select_rows_(design, row_start, nobs, out_cfg);
  x_step = select_columns_(x_step, step_active_columns_index_(step_model, out_cfg), out_cfg);
  if (options_.fit_intercept) {
    x_step = prepend_intercept_(x_step, out_cfg);
  }
  return x_step;  // NaN where a lagged value is not yet available
}

namespace {

// One forecast per horizon from the last observed origin. `design` was built from `nobs` observed rows (plus any future leading values); the row that holds
// the features of the last observation is the same for every horizon.
Series forecast_from_design_(const Series& design,
                             const LagPlan& plan,
                             const std::vector<DirectStepModel>& steps,
                             const DirectForecasterOptions& options,
                             std::int64_t nobs,
                             Config out_cfg) {
  const auto idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(out_cfg.platform).with_pinned(out_cfg.pinned);
  const std::int64_t origin_row = plan.max_lead + nobs;
  PLUSS_CHECK(origin_row < design.sizes()[0],
              "subsp::stats::ts::DirectForecaster::forecast: the model has no lags, so a forecast needs future values of its leading predictors");
  std::vector<Series> out_cols;
  out_cols.reserve(steps.size());
  for (const auto& step_model : steps) {
    const Series row_idx = pluss::make_series(std::vector<std::int64_t>{origin_row}, {1}, idx_cfg);
    Series x_row = design.index_select(0, row_idx);
    x_row = select_columns_(x_row, step_active_columns_index_(step_model, out_cfg), out_cfg);
    PLUSS_CHECK(x_row.isfinite().all().item<double>() == 1.0,
                "subsp::stats::ts::DirectForecaster::forecast: the history is too short for the lags of this model");
    if (options.fit_intercept) {
      x_row = prepend_intercept_(x_row, out_cfg);
    }
    out_cols.push_back(x_row.matmul(step_model.beta.reshape({step_model.beta.numel(), 1})));
  }
  if (out_cols.empty()) {
    return pluss::series::empty({0}, out_cfg);
  }
  if (out_cols.size() == 1) {
    return out_cols.front().reshape({1});
  }
  const Series forecast_2d = out_cols.front().unite(std::vector<Series>(out_cols.begin() + 1, out_cols.end()), 1);
  return forecast_2d.reshape({forecast_2d.sizes()[1]});
}

}  // namespace

Series DirectForecaster::forecast(const Series& x, const Series& y, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::forecast: model has no fitted steps");
  PLUSS_CHECK(!leading_requires_future_(lag_plan_),
              "subsp::stats::ts::DirectForecaster::forecast: the model uses future values of leading predictors; pass them as x_future");
  const auto out_cfg = work_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectForecaster::forecast").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  const Series design = build_lagged_predictors(x, yv, lag_plan_, out_cfg);
  return forecast_from_design_(design, lag_plan_, steps_, options_, nobs, out_cfg);
}

Series DirectForecaster::forecast(const Series& x, const Series& y, const Series& x_future, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::forecast: model has no fitted steps");
  const auto out_cfg = work_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectForecaster::forecast").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  const Series xf = as_matrix_(x_future, "DirectForecaster::forecast").to(out_cfg.platform, out_cfg.dtype);
  const Series xm = as_matrix_(x, "DirectForecaster::forecast").to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(xf.sizes()[1] == xm.sizes()[1], "subsp::stats::ts::DirectForecaster::forecast: x_future must have the same columns as x");
  const std::int64_t horizon_max = *std::max_element(lag_plan_.horizon.begin(), lag_plan_.horizon.end());
  PLUSS_CHECK(!leading_requires_future_(lag_plan_) || xf.sizes()[0] >= horizon_max,
              "subsp::stats::ts::DirectForecaster::forecast: x_future needs at least as many rows as the largest horizon");
  const std::int64_t extra = xf.sizes()[0];
  const Series x_ext = xm.unite({xf}, 0);
  const Series y_ext = yv.unite({pluss::series::full({extra}, std::numeric_limits<double>::quiet_NaN(), out_cfg)}, 0);
  const Series design = build_lagged_predictors(x_ext, y_ext, lag_plan_, out_cfg);
  return forecast_from_design_(design, lag_plan_, steps_, options_, nobs, out_cfg);
}

Series DirectForecaster::loss(const Series& x, const Series& y, RegressionLoss kind, Config cfg) const {
  const Config out_cfg = work_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectForecaster::loss").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  const Series pred = predict(x, yv, out_cfg);
  PLUSS_CHECK(pred.sizes()[0] == nobs, "subsp::stats::ts::DirectForecaster::loss: prediction length mismatch");

  const Series y_expanded = yv.reshape({nobs, 1}).expand(std::vector<std::int64_t>{nobs, pred.sizes()[1]});
  const Series residual = pred.sub(y_expanded);
  // Rows without a prediction (not enough history) or without an observed response are left out of the average.
  const Series usable = residual.isfinite().to(out_cfg.platform, ScalarType::Float64);
  const Series count = usable.sum(0, false).reshape({pred.sizes()[1]}).clamp_min(1.0);
  const Series clean = residual.nan_to_num(0.0, 0.0, 0.0);
  const Series per = (kind == RegressionLoss::MSE) ? clean.mul(clean).mul(usable) : clean.abs().mul(usable);
  return per.sum(0, false).reshape({pred.sizes()[1]}).div(count);
}

const DirectStepModel& DirectForecaster::step(std::int64_t i) const {
  PLUSS_CHECK(i >= 0 && i < static_cast<std::int64_t>(steps_.size()),
              "subsp::stats::ts::DirectForecaster::step: step index out of bounds");
  return steps_[static_cast<std::size_t>(i)];
}

} // namespace pluss::subsp::stats::ts
