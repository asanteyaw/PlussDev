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
  return pluss::arange(start, end_exclusive, idx_cfg);
}

std::vector<std::int64_t> active_columns_for_horizon_(const Series& mask, std::int64_t hidx) {
  const auto row = mask.select(0, hidx).to(pluss::pCPU, ScalarType::Bool).reshape({mask.sizes()[1]});
  const auto values = row.to_vec<std::uint8_t>();
  std::vector<std::int64_t> cols;
  cols.reserve(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (values[i] != 0) {
      cols.push_back(static_cast<std::int64_t>(i));
    }
  }
  return cols;
}

Series select_rows_(const Series& x, std::int64_t start, std::int64_t len, Config cfg) {
  PLUSS_CHECK(start >= 0, "subsp::stats::ts::DirectForecaster: row start must be non-negative");
  PLUSS_CHECK(len > 0, "subsp::stats::ts::DirectForecaster: row length must be positive");
  PLUSS_CHECK(start + len <= x.sizes()[0], "subsp::stats::ts::DirectForecaster: row slice exceeds bounds");
  return x.index_select(0, i64_range_(start, start + len, cfg));
}

Series select_columns_(const Series& x, const std::vector<std::int64_t>& cols, Config cfg) {
  PLUSS_CHECK(!cols.empty(), "subsp::stats::ts::DirectForecaster: expected non-empty active feature set");
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
  const Series col_idx = pluss::make_series(cols, {static_cast<std::int64_t>(cols.size())}, idx_cfg);
  return x.index_select(1, col_idx);
}

Series prepend_intercept_(const Series& x, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  const Series intercept = pluss::ones({x.sizes()[0], 1}, out_cfg);
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
      Series penalty = pluss::eye(x64.sizes()[1], x64.sizes()[1], cfg).mul(std::sqrt(opts.ridge_lambda));
      if (opts.fit_intercept) {
        const Series idx = pluss::make_series(std::vector<std::int64_t>{0},
                                              {1},
                                              Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform));
        penalty = pluss::index_put(penalty, std::vector<Series>{idx, idx}, pluss::zeros({1}, cfg), false);
      }
      x_aug = x64.unite({penalty}, 0);
      y_aug = y64.unite({pluss::zeros({x64.sizes()[1], 1}, cfg)}, 0);
    }
  }

  Series beta = Series::undefined();
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

Series concat_columns_(const std::vector<Series>& columns, std::int64_t rows, Config cfg) {
  const Config out_cfg = work_cfg_(cfg);
  if (columns.empty()) {
    return pluss::empty({rows, 0}, out_cfg);
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
  const Series mask = plan.prepared_predictors_per_horizon.to(pluss::pCPU, ScalarType::Bool);

  std::vector<DirectStepModel> steps;
  steps.reserve(plan.horizon.size());
  for (std::size_t i = 0; i < plan.horizon.size(); ++i) {
    const std::int64_t h = plan.horizon[i];
    const std::int64_t row_start = plan.max_lead - (h - 1);
    Series x_step = select_rows_(design, row_start, nobs, out_cfg);
    const auto cols = active_columns_for_horizon_(mask, static_cast<std::int64_t>(i));
    x_step = select_columns_(x_step, cols, out_cfg);
    if (options.fit_intercept) {
      x_step = prepend_intercept_(x_step, out_cfg);
    }
    x_step = x_step.nan_to_num(0.0, 0.0, 0.0);
    steps.push_back(DirectStepModel{
        .horizon = h,
        .active_columns = cols,
        .beta = fit_beta_(x_step, yv, options, out_cfg),
    });
  }

  return DirectForecaster(plan, options, std::move(steps));
}

Series DirectForecaster::predict(const Series& x, const Series& y, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::predict: model has no fitted steps");
  const Config out_cfg = work_cfg_(cfg);
  const std::int64_t nobs = observed_count_(x, y, lag_plan_);
  const Series yv = y.defined() ? as_vector_(y, "DirectForecaster::predict").to(out_cfg.platform, out_cfg.dtype)
                                : Series::undefined();

  const Series design = build_lagged_predictors(x, yv, lag_plan_, out_cfg);
  std::vector<Series> preds;
  preds.reserve(steps_.size());
  for (const auto& step_model : steps_) {
    const std::int64_t row_start = lag_plan_.max_lead - (step_model.horizon - 1);
    Series x_step = select_rows_(design, row_start, nobs, out_cfg);
    x_step = select_columns_(x_step, step_model.active_columns, out_cfg);
    if (options_.fit_intercept) {
      x_step = prepend_intercept_(x_step, out_cfg);
    }
    x_step = x_step.nan_to_num(0.0, 0.0, 0.0);
    preds.push_back(x_step.matmul(step_model.beta.reshape({step_model.beta.numel(), 1})));
  }
  return concat_columns_(preds, nobs, out_cfg);
}

Series DirectForecaster::prepared_predictors(const Series& x, const Series& y, std::int64_t horizon_step, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::prepared_predictors: model has no fitted steps");
  const auto out_cfg = work_cfg_(cfg);
  const auto& step_model = step_for_horizon_(steps_, horizon_step);
  const std::int64_t nobs = observed_count_(x, y, lag_plan_);
  const Series yv = y.defined() ? as_vector_(y, "DirectForecaster::prepared_predictors").to(out_cfg.platform, out_cfg.dtype)
                                : Series::undefined();
  const Series design = build_lagged_predictors(x, yv, lag_plan_, out_cfg);
  const std::int64_t row_start = lag_plan_.max_lead - (step_model.horizon - 1);
  Series x_step = select_rows_(design, row_start, nobs, out_cfg);
  x_step = select_columns_(x_step, step_model.active_columns, out_cfg);
  if (options_.fit_intercept) {
    x_step = prepend_intercept_(x_step, out_cfg);
  }
  return x_step.nan_to_num(0.0, 0.0, 0.0);
}

Series DirectForecaster::forecast(const Series& x, const Series& y, Config cfg) const {
  PLUSS_CHECK(!steps_.empty(), "subsp::stats::ts::DirectForecaster::forecast: model has no fitted steps");
  PLUSS_CHECK(!leading_requires_future_(lag_plan_),
              "subsp::stats::ts::DirectForecaster::forecast: future leading predictors are not supported yet; "
              "remove leading predictors or use predict with observed data");
  const auto out_cfg = work_cfg_(cfg);
  const auto idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(out_cfg.platform).with_pinned(out_cfg.pinned);
  const Series yv = as_vector_(y, "DirectForecaster::forecast").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  const Series design = build_lagged_predictors(x, yv, lag_plan_, out_cfg);

  std::vector<double> out;
  out.reserve(steps_.size());
  for (const auto& step_model : steps_) {
    const std::int64_t row_start = lag_plan_.max_lead - (step_model.horizon - 1);
    const std::int64_t row_idx_value = row_start + nobs - 1;
    const Series row_idx = pluss::make_series(std::vector<std::int64_t>{row_idx_value}, {1}, idx_cfg);
    Series x_row = design.index_select(0, row_idx);
    x_row = select_columns_(x_row, step_model.active_columns, out_cfg);
    if (options_.fit_intercept) {
      x_row = prepend_intercept_(x_row, out_cfg);
    }
    x_row = x_row.nan_to_num(0.0, 0.0, 0.0);
    out.push_back(x_row.matmul(step_model.beta.reshape({step_model.beta.numel(), 1})).item<double>());
  }
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
}

Series DirectForecaster::loss(const Series& x, const Series& y, RegressionLoss kind, Config cfg) const {
  const Config out_cfg = work_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectForecaster::loss").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  const Series pred = predict(x, yv, out_cfg);
  PLUSS_CHECK(pred.sizes()[0] == nobs, "subsp::stats::ts::DirectForecaster::loss: prediction length mismatch");

  const Series y_expanded = yv.reshape({nobs, 1}).expand(std::vector<std::int64_t>{nobs, pred.sizes()[1]});
  const Series residual = pred.sub(y_expanded);
  std::vector<double> out;
  out.reserve(static_cast<std::size_t>(pred.sizes()[1]));
  for (std::int64_t c = 0; c < pred.sizes()[1]; ++c) {
    const Series col = residual.select(1, c);
    if (kind == RegressionLoss::MSE) {
      out.push_back(col.mul(col).mean().item<double>());
    } else {
      out.push_back(col.abs().mean().item<double>());
    }
  }
  return pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, out_cfg);
}

const DirectStepModel& DirectForecaster::step(std::int64_t i) const {
  PLUSS_CHECK(i >= 0 && i < static_cast<std::int64_t>(steps_.size()),
              "subsp::stats::ts::DirectForecaster::step: step index out of bounds");
  return steps_[static_cast<std::size_t>(i)];
}

} // namespace pluss::subsp::stats::ts
