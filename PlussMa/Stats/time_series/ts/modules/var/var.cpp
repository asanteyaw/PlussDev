#include "pluss/subsp/stats/ts.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
#include <cmath>
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

constexpr double kLog2Pi = 1.83787706640934533908;

struct VarDesign {
  Series design = Series::undefined();
  Series target = Series::undefined();
  std::int64_t n = 0;
  std::int64_t t = 0;
  std::int64_t k = 0;
  std::int64_t p = 0;
  std::int64_t m = 0;
  std::int64_t d = 0;
};

struct ModelDims {
  std::int64_t p = 0;
  std::int64_t k = 0;
  std::int64_t m = 0;
};

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

Config cpu_f64_cfg_() {
  return Config{}.with_dtype(ScalarType::Float64);
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

std::vector<double> row_major_(const Series& x) {
  const ScalarType work_dtype = (x.platform() == pluss::Platform::MPS) ? ScalarType::Float32 : ScalarType::Float64;
  const Series flat = x.to(work_dtype).reshape({x.numel()}).contiguous();
  std::vector<double> out(static_cast<std::size_t>(flat.numel()), 0.0);
  if (flat.scalar_type() == ScalarType::Float32) {
    const auto* src = reinterpret_cast<const float*>(flat.raw_data());
    std::transform(src, src + flat.numel(), out.begin(), [](float v) { return static_cast<double>(v); });
  } else {
    const auto* src = reinterpret_cast<const double*>(flat.raw_data());
    std::copy(src, src + flat.numel(), out.begin());
  }
  return out;
}

std::int64_t idx2_(std::int64_t row, std::int64_t col, std::int64_t ncols) {
  return row * ncols + col;
}

VarDesign build_design_(const Series& y_in,
                        const Series* x_in,
                        std::int64_t lag_order,
                        bool include_intercept,
                        bool include_trend,
                        Config cfg,
                        const char* op_name) {
  PLUSS_CHECK(lag_order > 0, std::string("subsp::stats::ts::") + op_name + ": lag_order must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series y = as_matrix_(y_in, op_name).to(out_cfg.platform, out_cfg.dtype);
  const auto ys = y.sizes();
  const std::int64_t n = ys[0];
  const std::int64_t k = ys[1];
  PLUSS_CHECK(n > lag_order,
              std::string("subsp::stats::ts::") + op_name + ": expected observations > lag_order");
  PLUSS_CHECK(k > 0, std::string("subsp::stats::ts::") + op_name + ": expected at least one series");

  std::int64_t m = 0;
  Series x = Series::undefined();
  if (x_in != nullptr && x_in->defined()) {
    x = as_matrix_(*x_in, op_name).to(out_cfg.platform, out_cfg.dtype);
    const auto xs = x.sizes();
    PLUSS_CHECK(xs[0] == n,
                std::string("subsp::stats::ts::") + op_name + ": predictor row count must match y");
    m = xs[1];
  }

  const std::int64_t t = n - lag_order;
  const std::int64_t d = (include_intercept ? 1 : 0) + (include_trend ? 1 : 0) + lag_order * k + m;
  PLUSS_CHECK(d > 0, std::string("subsp::stats::ts::") + op_name + ": no predictors were constructed");

  const auto yv = row_major_(y);
  const auto xv = x.defined() ? row_major_(x) : std::vector<double>{};

  std::vector<double> design_values(static_cast<std::size_t>(t * d), 0.0);
  std::vector<double> target_values(static_cast<std::size_t>(t * k), 0.0);

  for (std::int64_t r = 0; r < t; ++r) {
    const std::int64_t src = r + lag_order;
    std::copy_n(yv.begin() + idx2_(src, 0, k), k, target_values.begin() + idx2_(r, 0, k));

    std::int64_t c = 0;
    if (include_intercept) {
      design_values[static_cast<std::size_t>(idx2_(r, c, d))] = 1.0;
      ++c;
    }
    if (include_trend) {
      design_values[static_cast<std::size_t>(idx2_(r, c, d))] = static_cast<double>(r + 1);
      ++c;
    }

    for (std::int64_t lag = 1; lag <= lag_order; ++lag) {
      const std::int64_t src_row = src - lag;
      std::copy_n(yv.begin() + idx2_(src_row, 0, k), k, design_values.begin() + idx2_(r, c, d));
      c += k;
    }

    if (m > 0) {
      std::copy_n(xv.begin() + idx2_(src, 0, m), m, design_values.begin() + idx2_(r, c, d));
    }
  }

  return VarDesign{
      .design = pluss::make_series(design_values, {t, d}, cpu_f64_cfg_()),
      .target = pluss::make_series(target_values, {t, k}, cpu_f64_cfg_()),
      .n = n,
      .t = t,
      .k = k,
      .p = lag_order,
      .m = m,
      .d = d,
  };
}

ModelDims validate_model_dims_(const VarFit& fit, const char* op_name) {
  PLUSS_CHECK(fit.ar.defined(), std::string("subsp::stats::ts::") + op_name + ": AR tensor is undefined");
  const auto ar_sizes = fit.ar.sizes();
  PLUSS_CHECK(ar_sizes.size() == 3, std::string("subsp::stats::ts::") + op_name + ": AR tensor must be [p,k,k]");
  PLUSS_CHECK(ar_sizes[0] > 0, std::string("subsp::stats::ts::") + op_name + ": lag_order must be > 0");
  PLUSS_CHECK(ar_sizes[1] == ar_sizes[2],
              std::string("subsp::stats::ts::") + op_name + ": AR tensor must have square per-lag matrices");

  const std::int64_t p = ar_sizes[0];
  const std::int64_t k = ar_sizes[1];
  const std::int64_t m = fit.beta.defined() ? as_matrix_(fit.beta, op_name).sizes()[0] : 0;

  PLUSS_CHECK(fit.constant.defined(), std::string("subsp::stats::ts::") + op_name + ": constant is undefined");
  PLUSS_CHECK(fit.trend.defined(), std::string("subsp::stats::ts::") + op_name + ": trend is undefined");
  PLUSS_CHECK(fit.constant.numel() == k,
              std::string("subsp::stats::ts::") + op_name + ": constant size must match number of series");
  PLUSS_CHECK(fit.trend.numel() == k,
              std::string("subsp::stats::ts::") + op_name + ": trend size must match number of series");

  if (m > 0) {
    const auto bs = as_matrix_(fit.beta, op_name).sizes();
    PLUSS_CHECK(bs[1] == k, std::string("subsp::stats::ts::") + op_name + ": beta must be [m,k]");
  }

  PLUSS_CHECK(fit.covariance.defined(), std::string("subsp::stats::ts::") + op_name + ": covariance is undefined");
  const auto cs = fit.covariance.sizes();
  PLUSS_CHECK(cs.size() == 2 && cs[0] == k && cs[1] == k,
              std::string("subsp::stats::ts::") + op_name + ": covariance must be [k,k]");
  return ModelDims{.p = p, .k = k, .m = m};
}

Series stabilize_cov_(Series covariance) {
  Series cov = covariance.to(ScalarType::Float64).contiguous();
  const auto sizes = cov.sizes();
  PLUSS_CHECK(sizes.size() == 2 && sizes[0] == sizes[1], "subsp::stats::ts::Var: covariance must be square");
  const std::int64_t k = sizes[0];
  Series eye = pluss::series::eye(k, k, cpu_f64_cfg_());
  double jitter = 1e-12;
  for (std::int64_t i = 0; i < 8; ++i) {
    const Series slogdet = pluss::subsp::math::linalg::slogdet(cov).to(ScalarType::Float64).reshape({2});
    const double sign = slogdet.select(0, 0).item<double>();
    const double logdet = slogdet.select(0, 1).item<double>();
    if (sign > 0.0 && std::isfinite(logdet)) {
      return cov;
    }
    cov = cov.add(eye.mul(jitter));
    jitter *= 10.0;
  }
  return cov;
}

Series compose_coeff_(const VarFit& fit, const ModelDims& dims) {
  const auto cpu_cfg = cpu_f64_cfg_();
  const auto c = row_major_(fit.constant.reshape({dims.k}));
  const auto tr = row_major_(fit.trend.reshape({dims.k}));
  const auto ar = row_major_(fit.ar.reshape({dims.p, dims.k, dims.k}));
  const auto beta = (dims.m > 0) ? row_major_(as_matrix_(fit.beta, "Var"))
                                 : std::vector<double>{};
  const std::int64_t d = (fit.include_intercept ? 1 : 0) + (fit.include_trend ? 1 : 0) + dims.p * dims.k + dims.m;
  std::vector<double> values(static_cast<std::size_t>(d * dims.k), 0.0);
  std::int64_t row = 0;
  if (fit.include_intercept) {
    std::copy(c.begin(), c.end(), values.begin());
    ++row;
  }
  if (fit.include_trend) {
    std::copy(tr.begin(), tr.end(), values.begin() + static_cast<std::ptrdiff_t>(row * dims.k));
    ++row;
  }

  for (std::int64_t lag = 0; lag < dims.p; ++lag) {
    for (std::int64_t src = 0; src < dims.k; ++src) {
      const std::int64_t ar_row = (lag * dims.k + src) * dims.k;
      const std::int64_t out_row = (row + lag * dims.k + src) * dims.k;
      std::copy_n(ar.begin() + ar_row, dims.k, values.begin() + static_cast<std::ptrdiff_t>(out_row));
    }
  }
  row += dims.p * dims.k;

  if (dims.m > 0) {
    std::copy(beta.begin(), beta.end(), values.begin() + static_cast<std::ptrdiff_t>(row * dims.k));
  }
  return pluss::make_series(values, {d, dims.k}, cpu_cfg);
}

Series infer_impl_(const VarFit& fit, const Series& y, const Series* x, Config cfg, const char* op_name) {
  const auto dims = validate_model_dims_(fit, op_name);
  if (dims.m > 0) {
    PLUSS_CHECK(x != nullptr && x->defined(),
                std::string("subsp::stats::ts::") + op_name + ": fitted model requires predictor matrix x");
  } else {
    PLUSS_CHECK(x == nullptr || !x->defined() || as_matrix_(*x, op_name).sizes()[1] == 0,
                std::string("subsp::stats::ts::") + op_name + ": fitted model has no predictor coefficients");
  }
  const auto design = build_design_(y, x, dims.p, fit.include_intercept, fit.include_trend, cfg, op_name);
  PLUSS_CHECK(design.k == dims.k,
              std::string("subsp::stats::ts::") + op_name + ": response dimension mismatch");
  PLUSS_CHECK(design.m == dims.m,
              std::string("subsp::stats::ts::") + op_name + ": predictor dimension mismatch");
  const Series coeff = compose_coeff_(fit, dims);
  return design.target.sub(design.design.matmul(coeff));
}

Series per_obs_loglik_(const Series& residuals, const Series& covariance) {
  const auto res = as_matrix_(residuals, "Var::loglik_per_obs").to(ScalarType::Float64).contiguous();
  const auto cov = stabilize_cov_(covariance);
  const auto inv_cov = pluss::subsp::math::linalg::pinverse(cov, 1e-12);
  const Series slogdet = pluss::subsp::math::linalg::slogdet(cov).to(ScalarType::Float64).reshape({2});
  const double sign = slogdet.select(0, 0).item<double>();
  const double logdet = slogdet.select(0, 1).item<double>();
  PLUSS_CHECK(sign > 0.0 && std::isfinite(logdet),
              "subsp::stats::ts::Var::loglik_per_obs: covariance must be positive definite");
  const double offset = -0.5 * (static_cast<double>(res.sizes()[1]) * kLog2Pi + logdet);
  return res.matmul(inv_cov).mul(res).sum(1, false).mul(-0.5).add(offset);
}

Series run_forecast_impl_(const VarFit& fit,
                          std::int64_t horizon,
                          const Series& y,
                          const Series* x_future,
                          Config cfg,
                          const char* op_name) {
  PLUSS_CHECK(horizon > 0, std::string("subsp::stats::ts::") + op_name + ": horizon must be > 0");
  const auto dims = validate_model_dims_(fit, op_name);
  const auto out_cfg = f64_cfg_(cfg);
  const auto y_mat = as_matrix_(y, op_name).to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(y_mat.sizes()[1] == dims.k,
              std::string("subsp::stats::ts::") + op_name + ": response dimension mismatch");
  PLUSS_CHECK(y_mat.sizes()[0] >= dims.p,
              std::string("subsp::stats::ts::") + op_name + ": expected at least lag_order observations");

  std::vector<double> xvals;
  if (dims.m > 0) {
    PLUSS_CHECK(x_future != nullptr && x_future->defined(),
                std::string("subsp::stats::ts::") + op_name + ": fitted model requires x_future");
    const auto xf = as_matrix_(*x_future, op_name).to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(xf.sizes()[1] == dims.m,
                std::string("subsp::stats::ts::") + op_name + ": x_future column mismatch");
    PLUSS_CHECK(xf.sizes()[0] >= horizon,
                std::string("subsp::stats::ts::") + op_name + ": x_future must have at least horizon rows");
    xvals = row_major_(xf);
  } else {
    PLUSS_CHECK(x_future == nullptr || !x_future->defined() || as_matrix_(*x_future, op_name).sizes()[1] == 0,
                std::string("subsp::stats::ts::") + op_name + ": model has no predictor coefficients");
  }

  const auto c = row_major_(fit.constant.reshape({dims.k}));
  const auto tr = row_major_(fit.trend.reshape({dims.k}));
  const auto ar = row_major_(fit.ar.reshape({dims.p, dims.k, dims.k}));
  const auto beta = (dims.m > 0) ? row_major_(as_matrix_(fit.beta, op_name))
                                 : std::vector<double>{};

  const auto y_hist = row_major_(y_mat.to(ScalarType::Float64));
  const std::int64_t n = y_mat.sizes()[0];
  std::vector<double> all;
  all.reserve(static_cast<std::size_t>((n + horizon) * dims.k));
  all.insert(all.end(), y_hist.begin(), y_hist.end());

  std::vector<double> out(static_cast<std::size_t>(horizon * dims.k), 0.0);
  for (std::int64_t h = 0; h < horizon; ++h) {
    const std::int64_t idx = n + h;
    const std::int64_t t_index = idx - dims.p + 1;
    for (std::int64_t eq = 0; eq < dims.k; ++eq) {
      double value = fit.include_intercept ? c[static_cast<std::size_t>(eq)] : 0.0;
      if (fit.include_trend) {
        value += tr[static_cast<std::size_t>(eq)] * static_cast<double>(t_index);
      }
      for (std::int64_t lag = 1; lag <= dims.p; ++lag) {
        const std::int64_t prev = idx - lag;
        const std::int64_t ar_base = (lag - 1) * dims.k * dims.k + eq;
        const auto* prev_row = all.data() + prev * dims.k;
        for (std::int64_t src = 0; src < dims.k; ++src) {
          value += prev_row[src] * ar[static_cast<std::size_t>(ar_base + src * dims.k)];
        }
      }
      if (dims.m > 0) {
        const auto* xrow = xvals.data() + h * dims.m;
        for (std::int64_t xm = 0; xm < dims.m; ++xm) {
          value += xrow[xm] * beta[static_cast<std::size_t>(xm * dims.k + eq)];
        }
      }
      out[static_cast<std::size_t>(h * dims.k + eq)] = value;
      all.push_back(value);
    }
  }
  return pluss::make_series(out, {horizon, dims.k}, out_cfg);
}

Series run_sim_impl_(const VarFit& fit, std::int64_t n, const Series* x, Config cfg, std::uint64_t seed, const char* op_name) {
  PLUSS_CHECK(n > 0, std::string("subsp::stats::ts::") + op_name + ": num_observations must be > 0");
  const auto dims = validate_model_dims_(fit, op_name);
  const auto out_cfg = f64_cfg_(cfg);

  std::vector<double> xvals;
  if (dims.m > 0) {
    PLUSS_CHECK(x != nullptr && x->defined(),
                std::string("subsp::stats::ts::") + op_name + ": fitted model requires predictor matrix x");
    const auto xm = as_matrix_(*x, op_name).to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(xm.sizes()[1] == dims.m, std::string("subsp::stats::ts::") + op_name + ": x column mismatch");
    PLUSS_CHECK(xm.sizes()[0] == n, std::string("subsp::stats::ts::") + op_name + ": x row count must equal num_observations");
    xvals = row_major_(xm);
  } else {
    PLUSS_CHECK(x == nullptr || !x->defined() || as_matrix_(*x, op_name).sizes()[1] == 0,
                std::string("subsp::stats::ts::") + op_name + ": model has no predictor coefficients");
  }

  const auto c = row_major_(fit.constant.reshape({dims.k}));
  const auto tr = row_major_(fit.trend.reshape({dims.k}));
  const auto ar = row_major_(fit.ar.reshape({dims.p, dims.k, dims.k}));
  const auto beta = (dims.m > 0) ? row_major_(as_matrix_(fit.beta, op_name))
                                 : std::vector<double>{};

  const Series cov = stabilize_cov_(fit.covariance);
  const Series chol = pluss::subsp::math::linalg::cholesky(cov).to(ScalarType::Float64).contiguous();
  const auto lvals = row_major_(chol);
  const auto z = row_major_(pluss::series::randn({n, dims.k}, cpu_f64_cfg_(), seed));

  std::vector<double> eps(static_cast<std::size_t>(n * dims.k), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    for (std::int64_t j = 0; j < dims.k; ++j) {
      double v = 0.0;
      for (std::int64_t r = 0; r < dims.k; ++r) {
        v += z[static_cast<std::size_t>(i * dims.k + r)] * lvals[static_cast<std::size_t>(j * dims.k + r)];
      }
      eps[static_cast<std::size_t>(i * dims.k + j)] = v;
    }
  }

  std::vector<double> yvals(static_cast<std::size_t>(n * dims.k), 0.0);
  for (std::int64_t i = 0; i < n; ++i) {
    for (std::int64_t eq = 0; eq < dims.k; ++eq) {
      double value = fit.include_intercept ? c[static_cast<std::size_t>(eq)] : 0.0;
      if (fit.include_trend) {
        value += tr[static_cast<std::size_t>(eq)] * static_cast<double>(i + 1);
      }
      for (std::int64_t lag = 1; lag <= dims.p; ++lag) {
        const std::int64_t prev = i - lag;
        if (prev < 0) {
          continue;
        }
        const std::int64_t ar_base = (lag - 1) * dims.k * dims.k + eq;
        const auto* prev_row = yvals.data() + prev * dims.k;
        for (std::int64_t src = 0; src < dims.k; ++src) {
          value += prev_row[src] * ar[static_cast<std::size_t>(ar_base + src * dims.k)];
        }
      }
      if (dims.m > 0) {
        const auto* xrow = xvals.data() + i * dims.m;
        for (std::int64_t xm = 0; xm < dims.m; ++xm) {
          value += xrow[xm] * beta[static_cast<std::size_t>(xm * dims.k + eq)];
        }
      }
      value += eps[static_cast<std::size_t>(i * dims.k + eq)];
      yvals[static_cast<std::size_t>(i * dims.k + eq)] = value;
    }
  }
  return pluss::make_series(yvals, {n, dims.k}, out_cfg);
}

VarFit fit_impl_(const Series& y, const Series* x, const VarOptions& options, Config cfg) {
  PLUSS_CHECK(options.lag_order > 0, "subsp::stats::ts::Var::fit_mle: lag_order must be > 0");
  PLUSS_CHECK(options.ridge >= 0.0, "subsp::stats::ts::Var::fit_mle: ridge must be >= 0");
  PLUSS_CHECK(options.pinv_rcond > 0.0, "subsp::stats::ts::Var::fit_mle: pinv_rcond must be > 0");

  const auto out_cfg = f64_cfg_(cfg);
  const auto design = build_design_(
      y, x, options.lag_order, options.include_intercept, options.include_trend, out_cfg, "Var::fit_mle");

  Series z_fit = design.design;
  Series y_fit = design.target;
  if (options.ridge > 0.0) {
    Series penalty = pluss::series::eye(design.d, design.d, cpu_f64_cfg_()).mul(std::sqrt(options.ridge));
    if (options.include_intercept || options.include_trend) {
      auto pvals = row_major_(penalty);
      const std::int64_t protected_cols = (options.include_intercept ? 1 : 0) + (options.include_trend ? 1 : 0);
      for (std::int64_t c = 0; c < protected_cols; ++c) {
        pvals[static_cast<std::size_t>(c * design.d + c)] = 0.0;
      }
      penalty = pluss::make_series(pvals, {design.d, design.d}, cpu_f64_cfg_());
    }
    z_fit = z_fit.unite({penalty}, 0);
    y_fit = y_fit.unite({pluss::series::zeros({design.d, design.k}, cpu_f64_cfg_())}, 0);
  }

  Series coeff = Series::undefined();
  const std::array<double, 5> scales{1.0, 10.0, 100.0, 1000.0, 1.0e6};
  for (double s : scales) {
    coeff = pluss::subsp::math::linalg::pinverse(z_fit, options.pinv_rcond * s).matmul(y_fit);
    if (coeff.isfinite().all().item<double>() == 1.0) {
      break;
    }
  }
  PLUSS_CHECK(coeff.isfinite().all().item<double>() == 1.0,
              "subsp::stats::ts::Var::fit_mle: failed to produce finite coefficients");

  const Series resid = design.target.sub(design.design.matmul(coeff));
  Series covariance = resid.transpose(0, 1).matmul(resid).div(static_cast<double>(std::max<std::int64_t>(1, design.t)));
  covariance = stabilize_cov_(covariance);

  const auto cv = row_major_(coeff);
  std::int64_t row = 0;
  std::vector<double> cvals(static_cast<std::size_t>(design.k), 0.0);
  std::vector<double> tvals(static_cast<std::size_t>(design.k), 0.0);
  if (options.include_intercept) {
    std::copy_n(cv.begin(), design.k, cvals.begin());
    ++row;
  }
  if (options.include_trend) {
    std::copy_n(cv.begin() + row * design.k, design.k, tvals.begin());
    ++row;
  }

  std::vector<double> ar_values(static_cast<std::size_t>(design.p * design.k * design.k), 0.0);
  for (std::int64_t lag = 0; lag < design.p; ++lag) {
    for (std::int64_t src = 0; src < design.k; ++src) {
      const std::int64_t from = (row + lag * design.k + src) * design.k;
      const std::int64_t to = (lag * design.k + src) * design.k;
      std::copy_n(cv.begin() + from, design.k, ar_values.begin() + to);
    }
  }
  row += design.p * design.k;

  std::vector<double> beta_values;
  if (design.m > 0) {
    beta_values.resize(static_cast<std::size_t>(design.m * design.k), 0.0);
    std::copy_n(cv.begin() + row * design.k, design.m * design.k, beta_values.begin());
  }

  VarFit fit;
  fit.lag_order = design.p;
  fit.include_intercept = options.include_intercept;
  fit.include_trend = options.include_trend;
  fit.constant = pluss::make_series(cvals, {design.k}, out_cfg);
  fit.trend = pluss::make_series(tvals, {design.k}, out_cfg);
  fit.ar = pluss::make_series(ar_values, {design.p, design.k, design.k}, out_cfg);
  fit.beta = (design.m > 0) ? pluss::make_series(beta_values, {design.m, design.k}, out_cfg)
                            : pluss::series::empty({0, design.k}, out_cfg);
  fit.covariance = covariance.to(out_cfg.platform, out_cfg.dtype);
  fit.sample_size = design.t;
  return fit;
}

Series ar_tensor_(const Series& ar, Config cfg, const char* op_name) {
  PLUSS_CHECK(ar.defined(), std::string("subsp::stats::ts::") + op_name + ": ar tensor is undefined");
  const auto ar3 = ar.to(ScalarType::Float64).contiguous();
  const auto sizes = ar3.sizes();
  PLUSS_CHECK(sizes.size() == 3, std::string("subsp::stats::ts::") + op_name + ": ar tensor must be [p,k,k]");
  PLUSS_CHECK(sizes[0] > 0, std::string("subsp::stats::ts::") + op_name + ": p must be > 0");
  PLUSS_CHECK(sizes[1] == sizes[2], std::string("subsp::stats::ts::") + op_name + ": ar tensor must be square per lag");
  return ar3.to(f64_cfg_(cfg).platform, f64_cfg_(cfg).dtype);
}

} // namespace

Series VarFit::infer(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  return infer_impl_(*this, y, nullptr, out_cfg, "VarFit::infer").to(out_cfg.platform, out_cfg.dtype);
}

Series VarFit::infer(const Series& y, const Series& x, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  return infer_impl_(*this, y, &x, out_cfg, "VarFit::infer").to(out_cfg.platform, out_cfg.dtype);
}

double VarFit::loglik(const Series& y) const {
  return loglik_per_obs(y, cpu_f64_cfg_()).sum().item<double>();
}

double VarFit::loglik(const Series& y, const Series& x) const {
  return loglik_per_obs(y, x, cpu_f64_cfg_()).sum().item<double>();
}

Series VarFit::loglik_per_obs(const Series& y, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series resid = infer_impl_(*this, y, nullptr, out_cfg, "VarFit::loglik_per_obs");
  return per_obs_loglik_(resid, covariance).to(out_cfg.platform, out_cfg.dtype);
}

Series VarFit::loglik_per_obs(const Series& y, const Series& x, Config cfg) const {
  const auto out_cfg = f64_cfg_(cfg);
  const Series resid = infer_impl_(*this, y, &x, out_cfg, "VarFit::loglik_per_obs");
  return per_obs_loglik_(resid, covariance).to(out_cfg.platform, out_cfg.dtype);
}

Series VarFit::forecast(std::int64_t horizon, const Series& y, Config cfg) const {
  return run_forecast_impl_(*this, horizon, y, nullptr, cfg, "VarFit::forecast");
}

Series VarFit::forecast(std::int64_t horizon, const Series& y, const Series& x_future, Config cfg) const {
  return run_forecast_impl_(*this, horizon, y, &x_future, cfg, "VarFit::forecast");
}

Series VarFit::simulate(std::int64_t num_observations, Config cfg, std::uint64_t seed) const {
  return run_sim_impl_(*this, num_observations, nullptr, cfg, seed, "VarFit::simulate");
}

Series VarFit::simulate(const Series& x, Config cfg, std::uint64_t seed) const {
  const auto xm = as_matrix_(x, "VarFit::simulate");
  return run_sim_impl_(*this, xm.sizes()[0], &x, cfg, seed, "VarFit::simulate");
}

VarFit Var::fit_mle(const Series& y, const VarOptions& options, Config cfg) {
  return fit_impl_(y, nullptr, options, cfg);
}

VarFit Var::fit_mle(const Series& y, const Series& x, const VarOptions& options, Config cfg) {
  return fit_impl_(y, &x, options, cfg);
}

std::pair<Series, Series> var_to_vecm(const Series& ar, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series ar3 = ar_tensor_(ar, out_cfg, "var_to_vecm");
  const auto sizes = ar3.sizes();
  const std::int64_t p = sizes[0];
  const std::int64_t k = sizes[1];
  const auto arv = row_major_(ar3);

  std::vector<double> impact(static_cast<std::size_t>(k * k), 0.0);
  for (std::int64_t lag = 0; lag < p; ++lag) {
    const std::int64_t base = lag * k * k;
    for (std::int64_t i = 0; i < k * k; ++i) {
      impact[static_cast<std::size_t>(i)] += arv[static_cast<std::size_t>(base + i)];
    }
  }
  for (std::int64_t i = 0; i < k; ++i) {
    impact[static_cast<std::size_t>(i * k + i)] -= 1.0;
  }

  if (p == 1) {
    return {
        pluss::series::empty({0, k, k}, out_cfg),
        pluss::make_series(impact, {k, k}, out_cfg),
    };
  }

  const std::int64_t q = p - 1;
  std::vector<double> short_run(static_cast<std::size_t>(q * k * k), 0.0);
  for (std::int64_t i = 1; i <= q; ++i) {
    const std::int64_t gbase = (i - 1) * k * k;
    for (std::int64_t j = i + 1; j <= p; ++j) {
      const std::int64_t abase = (j - 1) * k * k;
      for (std::int64_t u = 0; u < k * k; ++u) {
        short_run[static_cast<std::size_t>(gbase + u)] -= arv[static_cast<std::size_t>(abase + u)];
      }
    }
  }

  return {
      pluss::make_series(short_run, {q, k, k}, out_cfg),
      pluss::make_series(impact, {k, k}, out_cfg),
  };
}

Series vecm_to_var(const Series& short_run, const Series& impact, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const auto imp = as_matrix_(impact, "vecm_to_var").to(ScalarType::Float64).contiguous();
  const auto is = imp.sizes();
  PLUSS_CHECK(is[0] == is[1], "subsp::stats::ts::vecm_to_var: impact must be square");
  const std::int64_t k = is[0];

  Series sr = Series::undefined();
  std::int64_t q = 0;
  std::vector<double> srv;
  if (short_run.defined()) {
    sr = short_run.to(ScalarType::Float64).contiguous();
    const auto ss = sr.sizes();
    PLUSS_CHECK(ss.size() == 3, "subsp::stats::ts::vecm_to_var: short_run must be [q,k,k]");
    PLUSS_CHECK(ss[1] == k && ss[2] == k,
                "subsp::stats::ts::vecm_to_var: short_run dimensions must match impact");
    q = ss[0];
    srv = row_major_(sr);
  }
  const std::int64_t p = q + 1;
  const auto piv = row_major_(imp);

  std::vector<double> arv(static_cast<std::size_t>(p * k * k), 0.0);
  std::vector<double> suffix(static_cast<std::size_t>(k * k), 0.0);
  if (q > 0) {
    for (std::int64_t j = p; j >= 2; --j) {
      const std::int64_t g = j - 1;
      const std::int64_t gbase = (g - 1) * k * k;
      const std::int64_t abase = (j - 1) * k * k;
      for (std::int64_t u = 0; u < k * k; ++u) {
        const double fj = -srv[static_cast<std::size_t>(gbase + u)] - suffix[static_cast<std::size_t>(u)];
        arv[static_cast<std::size_t>(abase + u)] = fj;
        suffix[static_cast<std::size_t>(u)] += fj;
      }
    }
  }

  for (std::int64_t i = 0; i < k * k; ++i) {
    arv[static_cast<std::size_t>(i)] = piv[static_cast<std::size_t>(i)] - suffix[static_cast<std::size_t>(i)];
  }
  for (std::int64_t i = 0; i < k; ++i) {
    arv[static_cast<std::size_t>(i * k + i)] += 1.0;
  }

  return pluss::make_series(arv, {p, k, k}, out_cfg);
}

} // namespace pluss::subsp::stats::ts
