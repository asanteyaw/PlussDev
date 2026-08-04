#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace pluss::subsp::stats::ts {
namespace {

using pluss::Config;
using pluss::Platform;
using pluss::ScalarType;
using pluss::Series;

void require_defined_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::stats::ts::") + op + ": expected defined Series");
  PLUSS_CHECK(x.dim() == 1, std::string("subsp::stats::ts::") + op + ": expected 1D Series");
}

bool is_floating_(ScalarType dt) {
  return dt == ScalarType::Float16 || dt == ScalarType::BFloat16 || dt == ScalarType::Float32 ||
         dt == ScalarType::Float64;
}

ScalarType real_dtype_for_(const Series& x) {
  const auto dt = x.scalar_type();
  if (is_floating_(dt)) {
    if (dt == ScalarType::Float16 || dt == ScalarType::BFloat16) {
      return ScalarType::Float32;
    }
    if (x.platform() == Platform::MPS && dt == ScalarType::Float64) {
      return ScalarType::Float32;
    }
    return dt;
  }
  return x.platform() == Platform::MPS ? ScalarType::Float32 : ScalarType::Float64;
}

Config cfg_like_(const Series& x, ScalarType dt) {
  return Config{}.with_dtype(dt).with_platform(x.platform());
}

Series as_real_vector_(const Series& x, const char* op) {
  require_defined_vector_(x, op);
  const auto dt = real_dtype_for_(x);
  return x.to(dt).reshape({x.numel()});
}

Series nan_like_(const Series& x) {
  return pluss::series::full({x.numel()}, std::numeric_limits<double>::quiet_NaN(), cfg_like_(x, x.scalar_type()));
}

Series shift_nan_(const Series& x, std::int64_t periods) {
  const std::int64_t n = x.numel();
  if (n == 0) {
    return pluss::series::empty({0}, cfg_like_(x, x.scalar_type()));
  }
  if (periods == 0) {
    return x.clone();
  }

  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(x.platform());
  Series out = nan_like_(x);
  if (periods > 0) {
    if (periods >= n) {
      return out;
    }
    const Series src_idx = pluss::series::arange(0, n - periods, idx_cfg);
    const Series dst_idx = pluss::series::arange(periods, n, idx_cfg);
    const Series values = x.index_select(0, src_idx);
    return pluss::index_put(out, std::vector<Series>{dst_idx}, values, false);
  }

  const std::int64_t k = -periods;
  if (k >= n) {
    return out;
  }
  const Series src_idx = pluss::series::arange(k, n, idx_cfg);
  const Series dst_idx = pluss::series::arange(0, n - k, idx_cfg);
  const Series values = x.index_select(0, src_idx);
  return pluss::index_put(out, std::vector<Series>{dst_idx}, values, false);
}

std::string resample_op_name_(ResampleOp op) {
  switch (op) {
    case ResampleOp::Mean: return "mean";
    case ResampleOp::Sum: return "sum";
    case ResampleOp::Min: return "min";
    case ResampleOp::Max: return "max";
    case ResampleOp::First: return "first";
    case ResampleOp::Last: return "last";
    case ResampleOp::Count: return "count";
  }
  return "mean";
}

Series rolling_count_(const Series& x, std::int64_t window) {
  return pluss::series::ones({x.numel()}, cfg_like_(x, x.scalar_type())).stat.movsum(window);
}

Series apply_min_periods_(const Series& stat, const Series& x, std::int64_t window, std::int64_t min_periods) {
  if (min_periods == 1) {
    return stat;
  }
  const Series valid = rolling_count_(x, window).ge(static_cast<double>(min_periods));
  return stat.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

} // namespace

Series lag(const Series& x, std::int64_t periods) {
  if (periods < 0) {
    return lead(x, -periods);
  }
  const Series xr = as_real_vector_(x, "lag");
  return shift_nan_(xr, periods);
}

Series lead(const Series& x, std::int64_t periods) {
  if (periods < 0) {
    return lag(x, -periods);
  }
  const Series xr = as_real_vector_(x, "lead");
  return shift_nan_(xr, -periods);
}

Series diff(const Series& x, std::int64_t periods) {
  const Series xr = as_real_vector_(x, "diff");
  if (periods == 0) {
    return pluss::series::zeros({xr.numel()}, cfg_like_(xr, xr.scalar_type()));
  }
  if (periods > 0) {
    return xr.sub(lag(xr, periods));
  }
  return xr.sub(lead(xr, -periods));
}

Series returns(const Series& x, std::int64_t periods) {
  PLUSS_CHECK(periods != 0, "subsp::stats::ts::returns: periods cannot be zero");
  const Series xr = as_real_vector_(x, "returns");
  const Series base = periods > 0 ? lag(xr, periods) : lead(xr, -periods);
  const Series valid = xr.isfinite().logical_and(base.isfinite()).logical_and(base.ne(0.0));
  const Series raw = xr.div(base).sub(1.0);
  return raw.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

Series log_returns(const Series& x, std::int64_t periods) {
  PLUSS_CHECK(periods != 0, "subsp::stats::ts::log_returns: periods cannot be zero");
  const Series xr = as_real_vector_(x, "log_returns");
  const Series base = periods > 0 ? lag(xr, periods) : lead(xr, -periods);
  const Series valid = xr.isfinite().logical_and(base.isfinite()).logical_and(xr.gt(0.0)).logical_and(base.gt(0.0));
  const Series raw = xr.div(base).log();
  return raw.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

Series rolling_mean(const Series& x, std::int64_t window, std::int64_t min_periods) {
  PLUSS_CHECK(window > 0, "subsp::stats::ts::rolling_mean: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::rolling_mean: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window, "subsp::stats::ts::rolling_mean: min_periods cannot exceed window");
  const Series xr = as_real_vector_(x, "rolling_mean");
  return apply_min_periods_(xr.stat.movmean(window), xr, window, min_periods);
}

Series rolling_std(const Series& x, std::int64_t window, std::int64_t min_periods) {
  PLUSS_CHECK(window > 0, "subsp::stats::ts::rolling_std: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::rolling_std: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window, "subsp::stats::ts::rolling_std: min_periods cannot exceed window");
  const Series xr = as_real_vector_(x, "rolling_std");
  return apply_min_periods_(xr.stat.movstd(window), xr, window, min_periods);
}

Series rolling_sum(const Series& x, std::int64_t window, std::int64_t min_periods) {
  PLUSS_CHECK(window > 0, "subsp::stats::ts::rolling_sum: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::rolling_sum: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window, "subsp::stats::ts::rolling_sum: min_periods cannot exceed window");
  const Series xr = as_real_vector_(x, "rolling_sum");
  return apply_min_periods_(xr.stat.movsum(window), xr, window, min_periods);
}

Series rolling_var(const Series& x, std::int64_t window, std::int64_t min_periods) {
  PLUSS_CHECK(window > 0, "subsp::stats::ts::rolling_var: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::rolling_var: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window, "subsp::stats::ts::rolling_var: min_periods cannot exceed window");
  const Series xr = as_real_vector_(x, "rolling_var");
  return apply_min_periods_(xr.stat.movvar(window), xr, window, min_periods);
}

Series rolling_min(const Series& x, std::int64_t window, std::int64_t min_periods) {
  PLUSS_CHECK(window > 0, "subsp::stats::ts::rolling_min: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::rolling_min: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window, "subsp::stats::ts::rolling_min: min_periods cannot exceed window");
  const Series xr = as_real_vector_(x, "rolling_min");
  return apply_min_periods_(xr.stat.movmin(window), xr, window, min_periods);
}

Series rolling_max(const Series& x, std::int64_t window, std::int64_t min_periods) {
  PLUSS_CHECK(window > 0, "subsp::stats::ts::rolling_max: window must be > 0");
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::rolling_max: min_periods must be > 0");
  PLUSS_CHECK(min_periods <= window, "subsp::stats::ts::rolling_max: min_periods cannot exceed window");
  const Series xr = as_real_vector_(x, "rolling_max");
  return apply_min_periods_(xr.stat.movmax(window), xr, window, min_periods);
}

Series ema(const Series& x, double alpha) {
  PLUSS_CHECK(alpha > 0.0 && alpha <= 1.0, "subsp::stats::ts::ema: alpha must be in (0, 1]");
  const Series xr = as_real_vector_(x, "ema");
  const std::int64_t n = xr.numel();
  if (n <= 1 || alpha == 1.0) {
    return xr;
  }

  const double beta = 1.0 - alpha;
  const double log_beta = std::log(beta);
  const Series idx = pluss::series::arange(0, n, cfg_like_(xr, ScalarType::Int64)).to(xr.scalar_type());
  const Series beta_pow = idx.mul(log_beta).exp();
  const Series beta_pow_inv = idx.mul(-log_beta).exp();
  const Series weighted = xr.mul(beta_pow_inv);
  const Series prefix = weighted.cumsum(0);
  const Series x0 = xr.select(0, 0).reshape({1}).expand(std::vector<std::int64_t>{n});

  const Series ema_from_prefix = prefix.mul(beta_pow).mul(alpha);
  const Series seed_term = beta_pow.mul(1.0 - alpha).mul(x0);
  return ema_from_prefix.add(seed_term);
}

Series ewm_mean(const Series& x, double alpha) {
  return ema(x, alpha);
}

Series ewm_std(const Series& x, double alpha, double epsilon) {
  PLUSS_CHECK(epsilon > 0.0, "subsp::stats::ts::ewm_std: epsilon must be > 0");
  const Series xr = as_real_vector_(x, "ewm_std");
  const Series mean = ema(xr, alpha);
  const Series mean_sq = ema(xr.square(), alpha);
  const Series var = mean_sq.sub(mean.square()).clamp(0.0, 1e308);
  return var.add(epsilon).sqrt();
}

Series expanding_mean(const Series& x, std::int64_t min_periods) {
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::expanding_mean: min_periods must be > 0");
  const Series xr = as_real_vector_(x, "expanding_mean");
  const std::int64_t n = xr.numel();
  if (n == 0) {
    return xr;
  }
  const Series idx = pluss::series::arange(0, n, cfg_like_(xr, ScalarType::Int64)).to(xr.scalar_type());
  const Series count = idx.add(1.0);
  const Series mean = xr.cumsum(0).div(count);
  if (min_periods == 1) {
    return mean;
  }
  const Series valid = count.ge(static_cast<double>(min_periods));
  return mean.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

Series expanding_std(const Series& x, std::int64_t min_periods, double epsilon) {
  PLUSS_CHECK(min_periods > 0, "subsp::stats::ts::expanding_std: min_periods must be > 0");
  PLUSS_CHECK(epsilon > 0.0, "subsp::stats::ts::expanding_std: epsilon must be > 0");
  const Series xr = as_real_vector_(x, "expanding_std");
  const std::int64_t n = xr.numel();
  if (n == 0) {
    return xr;
  }
  const Series idx = pluss::series::arange(0, n, cfg_like_(xr, ScalarType::Int64)).to(xr.scalar_type());
  const Series count = idx.add(1.0);
  const Series mean = xr.cumsum(0).div(count);
  const Series mean_sq = xr.square().cumsum(0).div(count);
  const Series var = mean_sq.sub(mean.square()).clamp(0.0, 1e308);
  const Series stdv = var.add(epsilon).sqrt();
  if (min_periods == 1) {
    return stdv;
  }
  const Series valid = count.ge(static_cast<double>(min_periods));
  return stdv.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

Series zscore_rolling(const Series& x, std::int64_t window, std::int64_t min_periods, double epsilon) {
  PLUSS_CHECK(epsilon > 0.0, "subsp::stats::ts::zscore_rolling: epsilon must be > 0");
  const Series xr = as_real_vector_(x, "zscore_rolling");
  const Series mean = rolling_mean(xr, window, min_periods);
  const Series stdv = rolling_std(xr, window, min_periods);
  const Series denom_valid = stdv.abs().gt(epsilon).logical_and(stdv.isfinite());
  const Series safe_denom = stdv.masked_fill(denom_valid.logical_not(), epsilon);
  const Series raw = xr.sub(mean).div(safe_denom);
  const Series valid = denom_valid.logical_and(mean.isfinite()).logical_and(xr.isfinite());
  return raw.masked_fill(valid.logical_not(), std::numeric_limits<double>::quiet_NaN());
}

Series fill_gaps(const Series& x, GapFillMode mode, std::optional<double> constant) {
  const Series xr = as_real_vector_(x, "fill_gaps");
  const std::int64_t n = xr.numel();
  if (n == 0) {
    return xr;
  }

  const Series missing = xr.isnan();
  if (mode == GapFillMode::Zero) {
    return xr.masked_fill(missing, 0.0);
  }
  if (mode == GapFillMode::Constant) {
    PLUSS_CHECK(constant.has_value(), "subsp::stats::ts::fill_gaps: constant mode requires a value");
    return xr.masked_fill(missing, *constant);
  }

  Series out = xr;
  for (std::int64_t step = 1; step < n; step *= 2) {
    const Series shifted = (mode == GapFillMode::Forward) ? shift_nan_(out, step) : shift_nan_(out, -step);
    const Series can_fill = out.isnan().logical_and(shifted.isnan().logical_not());
    out = out.masked_fill(can_fill, 0.0).add(shifted.masked_fill(can_fill.logical_not(), 0.0));
  }
  return out;
}

ResampleResult resample(const pluss::TimeIndex& index,
                        const Series& values,
                        std::string_view freq,
                        ResampleOp op,
                        std::int64_t step) {
  PLUSS_CHECK(values.defined(), "subsp::stats::ts::resample: expected defined Series values");
  PLUSS_CHECK(values.dim() == 1, "subsp::stats::ts::resample: expected 1D values");
  PLUSS_CHECK(index.size() == values.numel(), "subsp::stats::ts::resample: index/values length mismatch");
  PLUSS_CHECK(step > 0, "subsp::stats::ts::resample: step must be > 0");
  PLUSS_CHECK(!freq.empty(), "subsp::stats::ts::resample: freq cannot be empty");

  const Series xr = as_real_vector_(values, "resample");
  ResampleResult out{};
  out.labels = index.resample_bins(freq, step).labels;
  if (op == ResampleOp::Count) {
    out.values = index.resample_count(freq, step);
    return out;
  }
  out.values = index.resample_agg(xr, freq, resample_op_name_(op), step);
  return out;
}

} // namespace pluss::subsp::stats::ts
