#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config i64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(cfg.platform).with_pinned(cfg.pinned);
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

Series select_rows_(const Series& x, const Series& idx, Config cfg) {
  return x.index_select(0, idx.to(i64_cfg_(cfg).platform, i64_cfg_(cfg).dtype));
}

std::int64_t required_observations_(const DirectForecasterOptions& options, std::int64_t num_predictors) {
  const LagPlan plan = build_lag_plan(num_predictors, options.lag);
  const std::int64_t max_h = plan.horizon.empty() ? 0 : *std::max_element(plan.horizon.begin(), plan.horizon.end());
  return plan.max_lag + max_h;
}

Series average_with_counts_(const Series& sum, const Series& count, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series count_f64 = count.to(out_cfg.platform, out_cfg.dtype);
  const Series valid = count_f64.gt(0.0);
  const Series safe_denom = count_f64.where(valid, pluss::ones(count_f64.sizes(), out_cfg));
  const Series mean = sum.to(out_cfg.platform, out_cfg.dtype).div(safe_denom);
  const Series nan_fill = pluss::full(sum.sizes(), std::numeric_limits<double>::quiet_NaN(), out_cfg);
  return mean.where(valid, nan_fill);
}

Series loss_by_column_(const Series& pred, const Series& y, RegressionLoss kind, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const std::int64_t n = pred.sizes()[0];
  const std::int64_t h = pred.sizes()[1];
  const Series y_expanded = y.reshape({n, 1}).expand(std::vector<std::int64_t>{n, h});
  const Series residual = pred.sub(y_expanded);
  const Series value = (kind == RegressionLoss::MSE) ? residual.mul(residual) : residual.abs();
  const Series den = value.isfinite().to(out_cfg.platform, out_cfg.dtype).sum(0, false).reshape({h});
  const Series num = value.nan_to_num(0.0, 0.0, 0.0).sum(0, false).reshape({h});
  const Series valid = den.gt(0.0);
  const Series safe_denom = den.where(valid, pluss::ones({h}, out_cfg));
  const Series mean = num.div(safe_denom);
  const Series nan_fill = pluss::full({h}, std::numeric_limits<double>::quiet_NaN(), out_cfg);
  return mean.where(valid, nan_fill);
}

} // namespace

DirectCVForecaster::DirectCVForecaster(TsPartition partition,
                                       DirectForecasterOptions options,
                                       std::vector<DirectForecaster> models)
    : partition_(std::move(partition)),
      options_(std::move(options)),
      models_(std::move(models)) {}

DirectCVForecaster DirectCVForecaster::fit(const Series& x,
                                           const Series& y,
                                           const TsPartition& partition,
                                           const DirectForecasterOptions& options,
                                           Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto idx_cfg = i64_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectCVForecaster::fit").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  PLUSS_CHECK(nobs == partition.num_observations(),
              "subsp::stats::ts::DirectCVForecaster::fit: partition observation count must match Y");

  Series xm = Series::undefined();
  std::int64_t num_predictors = 0;
  if (x.defined()) {
    xm = as_matrix_(x, "DirectCVForecaster::fit").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(xm.sizes()[0] == nobs,
                "subsp::stats::ts::DirectCVForecaster::fit: X and Y must have equal observation counts");
    num_predictors = xm.sizes()[1];
  }

  const std::int64_t min_required = required_observations_(options, num_predictors);
  for (std::int64_t i = 0; i < partition.num_test_sets(); ++i) {
    const auto& w = partition.window(i);
    PLUSS_CHECK(w.train_size() >= min_required,
                "subsp::stats::ts::DirectCVForecaster::fit: train window too short for lag/horizon configuration");
    PLUSS_CHECK(w.test_size() >= min_required,
                "subsp::stats::ts::DirectCVForecaster::fit: test window too short for lag/horizon configuration");
  }

  std::vector<DirectForecaster> models;
  models.reserve(static_cast<std::size_t>(partition.num_test_sets()));
  for (std::int64_t i = 0; i < partition.num_test_sets(); ++i) {
    const Series train_idx = partition.training(i, idx_cfg);
    const Series y_train = select_rows_(yv, train_idx, out_cfg).reshape({train_idx.numel()});
    const Series x_train = xm.defined() ? select_rows_(xm, train_idx, out_cfg) : Series::undefined();
    models.push_back(DirectForecaster::fit(x_train, y_train, options, out_cfg));
  }
  return DirectCVForecaster(partition, options, std::move(models));
}

Series DirectCVForecaster::cvpredict(const Series& x, const Series& y, Config cfg) const {
  PLUSS_CHECK(!models_.empty(), "subsp::stats::ts::DirectCVForecaster::cvpredict: no fold models are available");
  const auto out_cfg = f64_cfg_(cfg);
  const auto idx_cfg = i64_cfg_(cfg);

  const Series yv = as_vector_(y, "DirectCVForecaster::cvpredict").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  PLUSS_CHECK(nobs == partition_.num_observations(),
              "subsp::stats::ts::DirectCVForecaster::cvpredict: partition observation count must match Y");
  Series xm = Series::undefined();
  if (x.defined()) {
    xm = as_matrix_(x, "DirectCVForecaster::cvpredict").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(xm.sizes()[0] == nobs,
                "subsp::stats::ts::DirectCVForecaster::cvpredict: X and Y must have equal observation counts");
  }

  const std::int64_t h = models_.front().num_steps();
  Series sum = pluss::zeros({nobs, h}, out_cfg);
  Series cnt = pluss::zeros({nobs, h}, out_cfg);

  for (std::int64_t i = 0; i < partition_.num_test_sets(); ++i) {
    const Series test_idx = partition_.test(i, idx_cfg);
    const Series pred_all = models_[static_cast<std::size_t>(i)].predict(xm, yv, out_cfg);
    const Series pred_test = pred_all.index_select(0, test_idx);
    const Series old_sum = sum.index_select(0, test_idx);
    sum = pluss::index_put(sum, std::vector<Series>{test_idx}, old_sum.add(pred_test), false);

    const Series ones = pluss::ones({test_idx.numel(), h}, out_cfg);
    const Series old_cnt = cnt.index_select(0, test_idx);
    cnt = pluss::index_put(cnt, std::vector<Series>{test_idx}, old_cnt.add(ones), false);
  }

  return average_with_counts_(sum, cnt, out_cfg);
}

Series DirectCVForecaster::cvloss(const Series& x, const Series& y, CVLossMode mode, RegressionLoss kind, Config cfg) const {
  PLUSS_CHECK(!models_.empty(), "subsp::stats::ts::DirectCVForecaster::cvloss: no fold models are available");
  const auto out_cfg = f64_cfg_(cfg);
  const auto idx_cfg = i64_cfg_(cfg);
  const Series yv = as_vector_(y, "DirectCVForecaster::cvloss").to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t nobs = yv.numel();
  PLUSS_CHECK(nobs == partition_.num_observations(),
              "subsp::stats::ts::DirectCVForecaster::cvloss: partition observation count must match Y");
  Series xm = Series::undefined();
  if (x.defined()) {
    xm = as_matrix_(x, "DirectCVForecaster::cvloss").to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(xm.sizes()[0] == nobs,
                "subsp::stats::ts::DirectCVForecaster::cvloss: X and Y must have equal observation counts");
  }

  if (mode == CVLossMode::Average) {
    return loss_by_column_(cvpredict(xm, yv, out_cfg), yv, kind, out_cfg);
  }

  const std::int64_t f = partition_.num_test_sets();
  const std::int64_t h = models_.front().num_steps();
  std::vector<Series> fold_losses;
  fold_losses.reserve(static_cast<std::size_t>(f));
  for (std::int64_t i = 0; i < f; ++i) {
    const Series test_idx = partition_.test(i, idx_cfg);
    const Series pred_all = models_[static_cast<std::size_t>(i)].predict(xm, yv, out_cfg);
    const Series pred_test = pred_all.index_select(0, test_idx);
    const Series y_test = yv.index_select(0, test_idx);
    fold_losses.push_back(loss_by_column_(pred_test, y_test, kind, out_cfg).reshape({1, h}));
  }
  if (fold_losses.empty()) {
    return pluss::empty({0, h}, out_cfg);
  }
  if (fold_losses.size() == 1) {
    return fold_losses.front();
  }
  return fold_losses.front().unite(std::vector<Series>(fold_losses.begin() + 1, fold_losses.end()), 0);
}

std::int64_t DirectCVForecaster::num_steps() const noexcept {
  if (models_.empty()) {
    return 0;
  }
  return models_.front().num_steps();
}

const DirectForecaster& DirectCVForecaster::fold_model(std::int64_t i) const {
  PLUSS_CHECK(i >= 0 && i < static_cast<std::int64_t>(models_.size()),
              "subsp::stats::ts::DirectCVForecaster::fold_model: fold index out of bounds");
  return models_[static_cast<std::size_t>(i)];
}

} // namespace pluss::subsp::stats::ts
