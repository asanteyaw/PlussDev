#include "pluss/subsp/stats/inference/hypothesis_testing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/distr.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace pluss::subsp::stats::inference::hypothesis {
namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

constexpr double kEps = 1e-14;

Config f64_cfg_(Config cfg) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(cfg.platform).with_pinned(cfg.pinned);
}

double clamp01_(double p) {
  return std::clamp(p, 0.0, 1.0);
}

void check_alpha_(double alpha, const char* op) {
  PLUSS_CHECK(std::isfinite(alpha) && alpha > 0.0 && alpha < 1.0,
              std::string(op) + ": alpha must be in (0,1)");
}

Series as_vector_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string(op) + ": expected defined Series");
  const auto s = x.sizes();
  PLUSS_CHECK(!s.empty(), std::string(op) + ": expected non-scalar Series");
  if (s.size() == 1) {
    return x.reshape({s[0]});
  }
  PLUSS_CHECK(s.size() == 2, std::string(op) + ": expected vector or matrix with singleton dimension");
  PLUSS_CHECK(s[0] == 1 || s[1] == 1, std::string(op) + ": expected vector input");
  return x.reshape({x.numel()});
}

double scalar_(const Series& x) {
  return x.to(ScalarType::Float64).reshape({1}).item<double>();
}

Series finite_values_sorted_(const Series& x, const char* op, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series raw = as_vector_(x, op);
  const Series base = raw.to(out_cfg.platform, out_cfg.dtype).reshape({raw.numel()});
  return base.masked_select(base.isfinite()).sort(0, false, false);
}

double normal_cdf_(double x, double mean, double stddev, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::Normal{mean, stddev}.cdf(q).item<double>());
}

double chisq_cdf_(double x, std::int64_t dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::ChiSquare{dof}.cdf(q).item<double>());
}

double chisq_quantile_(double p, std::int64_t dof, Config cfg) {
  if (p <= 0.0) {
    return 0.0;
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = 0.0;
  double hi = std::max(1.0, static_cast<double>(dof) + 20.0 * std::sqrt(static_cast<double>(dof)));
  for (std::int64_t i = 0; i < 80 && chisq_cdf_(hi, dof, cfg) < p; ++i) {
    hi *= 2.0;
  }
  for (std::int64_t i = 0; i < 140; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (chisq_cdf_(mid, dof, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double ks_two_sided_pvalue_(double n_eff, double d) {
  if (d <= 0.0) {
    return 1.0;
  }
  const double root_n = std::sqrt(std::max(kEps, n_eff));
  const double lambda = (root_n + 0.12 + 0.11 / root_n) * d;
  double sum = 0.0;
  for (std::int64_t j = 1; j <= 200; ++j) {
    const double term = std::exp(-2.0 * static_cast<double>(j * j) * lambda * lambda);
    const double signed_term = ((j % 2) == 1 ? 1.0 : -1.0) * term;
    sum += signed_term;
    if (term < 1e-12) {
      break;
    }
  }
  return clamp01_(2.0 * sum);
}

double ks_one_sided_pvalue_(double n_eff, double d) {
  if (d <= 0.0) {
    return 1.0;
  }
  return clamp01_(std::exp(-2.0 * n_eff * d * d));
}

double ks_critical_two_sided_(double n_eff, double alpha) {
  double lo = 0.0;
  double hi = 1.0;
  for (std::int64_t i = 0; i < 100; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (ks_two_sided_pvalue_(n_eff, mid) > alpha) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double ks_critical_one_sided_(double n_eff, double alpha) {
  return std::sqrt(std::max(0.0, -0.5 * std::log(alpha) / std::max(kEps, n_eff)));
}

} // namespace

KSTestResult kstest(const Series& x,
                    double mean0,
                    double stddev0,
                    const TestOptions& options,
                    Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::kstest";
  check_alpha_(options.alpha, op);
  PLUSS_CHECK(std::isfinite(stddev0) && stddev0 > 0.0, std::string(op) + ": stddev0 must be > 0");

  const auto out_cfg = f64_cfg_(cfg);
  const Series values = finite_values_sorted_(x, op, cfg);
  const std::int64_t n = values.numel();
  PLUSS_CHECK(n > 0, std::string(op) + ": no finite observations");

  const Series fi = pluss::subsp::stats::distr::Normal{mean0, stddev0}.cdf(values).clamp(0.0, 1.0);
  const Series ecdf_hi = Series::arange(1, n + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).div(static_cast<double>(n));
  const Series ecdf_lo = Series::arange(0, n, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).div(static_cast<double>(n));
  const double dplus = std::max(0.0, scalar_(ecdf_hi.sub(fi).max()));
  const double dminus = std::max(0.0, scalar_(fi.sub(ecdf_lo).max()));

  double stat = std::max(dplus, dminus);
  if (options.tail == Tail::Right) {
    stat = dplus;
  } else if (options.tail == Tail::Left) {
    stat = dminus;
  }

  const double n_eff = static_cast<double>(n);
  const double pvalue = (options.tail == Tail::Both)
                            ? ks_two_sided_pvalue_(n_eff, stat)
                            : ks_one_sided_pvalue_(n_eff, stat);

  const double critical = (options.tail == Tail::Both)
                              ? ks_critical_two_sided_(n_eff, options.alpha)
                              : ks_critical_one_sided_(n_eff, options.alpha);

  KSTestResult out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = stat > critical;
  out.d_plus = dplus;
  out.d_minus = dminus;
  out.mean0 = mean0;
  out.stddev0 = stddev0;
  out.effective_n = n;
  return out;
}

KSTest2Result kstest2(const Series& x,
                      const Series& y,
                      const TestOptions& options,
                      Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::kstest2";
  check_alpha_(options.alpha, op);

  const auto out_cfg = f64_cfg_(cfg);
  const Series xvals = finite_values_sorted_(x, op, cfg);
  const Series yvals = finite_values_sorted_(y, op, cfg);
  const std::int64_t nx = xvals.numel();
  const std::int64_t ny = yvals.numel();
  PLUSS_CHECK(nx > 0 && ny > 0, std::string(op) + ": requires nonempty finite samples");

  const Series fx_x = Series::arange(1, nx + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).div(static_cast<double>(nx));
  const Series fy_y = Series::arange(1, ny + 1, 1, ScalarType::Int64).to(out_cfg.platform, out_cfg.dtype).div(static_cast<double>(ny));
  const Series fy_x = yvals.reshape({1, ny}).le(xvals.reshape({nx, 1})).to(out_cfg.dtype).sum(1, false).div(static_cast<double>(ny));
  const Series fx_y = xvals.reshape({1, nx}).le(yvals.reshape({ny, 1})).to(out_cfg.dtype).sum(1, false).div(static_cast<double>(nx));

  const double dplus = std::max(
      std::max(0.0, scalar_(fx_x.sub(fy_x).max())),
      std::max(0.0, scalar_(fx_y.sub(fy_y).max())));
  const double dminus = std::max(
      std::max(0.0, scalar_(fy_x.sub(fx_x).max())),
      std::max(0.0, scalar_(fy_y.sub(fx_y).max())));

  double stat = std::max(dplus, dminus);
  if (options.tail == Tail::Right) {
    stat = dplus;
  } else if (options.tail == Tail::Left) {
    stat = dminus;
  }

  const double n_eff = (static_cast<double>(nx) * static_cast<double>(ny)) /
                       static_cast<double>(nx + ny);
  const double pvalue = (options.tail == Tail::Both)
                            ? ks_two_sided_pvalue_(n_eff, stat)
                            : ks_one_sided_pvalue_(n_eff, stat);
  const double critical = (options.tail == Tail::Both)
                              ? ks_critical_two_sided_(n_eff, options.alpha)
                              : ks_critical_one_sided_(n_eff, options.alpha);

  KSTest2Result out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = stat > critical;
  out.d_plus = dplus;
  out.d_minus = dminus;
  out.effective_nx = nx;
  out.effective_ny = ny;
  return out;
}

JBTestResult jbtest(const Series& x, double alpha, Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::jbtest";
  check_alpha_(alpha, op);

  const auto out_cfg = f64_cfg_(cfg);
  const Series values = finite_values_sorted_(x, op, cfg);
  const std::int64_t n = values.numel();
  PLUSS_CHECK(n >= 4, std::string(op) + ": requires at least 4 finite observations");

  const double nd = static_cast<double>(n);
  const double mean = scalar_(values.mean());
  const Series centered = values.sub(mean);
  const double m2 = scalar_(centered.mul(centered).mean());
  const double m3 = scalar_(centered.mul(centered).mul(centered).mean());
  const double m4 = scalar_(centered.mul(centered).mul(centered).mul(centered).mean());
  PLUSS_CHECK(m2 > kEps, std::string(op) + ": variance too small");

  const double skew = m3 / std::pow(m2, 1.5);
  const double kurt = m4 / (m2 * m2);
  const double jb = nd * (skew * skew + 0.25 * (kurt - 3.0) * (kurt - 3.0)) / 6.0;

  const double cdf = chisq_cdf_(jb, 2, out_cfg);
  const double pvalue = 1.0 - cdf;
  const double critical = chisq_quantile_(1.0 - alpha, 2, out_cfg);

  JBTestResult out;
  out.stat = jb;
  out.pvalue = clamp01_(pvalue);
  out.critical = critical;
  out.reject = jb > critical;
  out.skewness = skew;
  out.kurtosis = kurt;
  out.effective_n = n;
  return out;
}

ChiGofResult chisq_gof(const Series& observed,
                       const Series& expected,
                       const ChiGofOptions& options,
                       Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::chisq_gof";
  check_alpha_(options.alpha, op);

  const auto out_cfg = f64_cfg_(cfg);
  const Series obs = as_vector_(observed, op).to(out_cfg.platform, out_cfg.dtype);
  const std::int64_t k = obs.numel();
  PLUSS_CHECK(k >= 2, std::string(op) + ": need at least two bins");
  PLUSS_CHECK(scalar_(obs.isfinite().all()) > 0.5, std::string(op) + ": observed must be finite");
  PLUSS_CHECK(scalar_(obs.ge(0.0).all()) > 0.5, std::string(op) + ": observed must be non-negative");

  Series expv = pluss::series::undefined();
  if (expected.defined()) {
    expv = as_vector_(expected, op).to(out_cfg.platform, out_cfg.dtype);
    PLUSS_CHECK(expv.numel() == k, std::string(op) + ": expected must have same length as observed");
    PLUSS_CHECK(scalar_(expv.isfinite().all()) > 0.5, std::string(op) + ": expected must be finite");
    PLUSS_CHECK(scalar_(expv.gt(0.0).all()) > 0.5, std::string(op) + ": expected bins must be > 0");
  } else {
    const double total = scalar_(obs.sum());
    expv = pluss::series::full({k}, total / static_cast<double>(k), out_cfg);
  }

  if (options.normalize_expected) {
    const double sum_exp = scalar_(expv.sum());
    const double sum_obs = scalar_(obs.sum());
    PLUSS_CHECK(sum_exp > kEps, std::string(op) + ": expected sum must be > 0");
    expv = expv.mul(sum_obs / sum_exp);
  }

  PLUSS_CHECK(scalar_(expv.gt(0.0).all()) > 0.5, std::string(op) + ": expected bins must be > 0");
  const double dof = static_cast<double>(k - 1) - options.ddof;
  PLUSS_CHECK(dof > 0.0, std::string(op) + ": non-positive degrees of freedom");

  const double stat = scalar_(obs.sub(expv).mul(obs.sub(expv)).div(expv).sum());
  const double pvalue = clamp01_(1.0 - chisq_cdf_(stat, static_cast<std::int64_t>(std::llround(dof)), out_cfg));
  const double critical = chisq_quantile_(1.0 - options.alpha, static_cast<std::int64_t>(std::llround(dof)), out_cfg);

  ChiGofResult out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = stat > critical;
  out.dof = dof;
  out.total_count = scalar_(obs.sum());
  out.bins = k;
  out.expected = expv;
  return out;
}

ChiIndepResult chisq_indep(const Series& contingency, double alpha, Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::chisq_indep";
  check_alpha_(alpha, op);

  const auto out_cfg = f64_cfg_(cfg);
  const auto s = contingency.sizes();
  PLUSS_CHECK(contingency.defined(), std::string(op) + ": expected defined contingency table");
  PLUSS_CHECK(s.size() == 2, std::string(op) + ": expected 2D contingency table");
  const std::int64_t r = s[0];
  const std::int64_t c = s[1];
  PLUSS_CHECK(r >= 2 && c >= 2, std::string(op) + ": contingency table must be at least 2x2");

  const Series obs = contingency.to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(scalar_(obs.isfinite().all()) > 0.5, std::string(op) + ": contingency values must be finite");
  PLUSS_CHECK(scalar_(obs.ge(0.0).all()) > 0.5, std::string(op) + ": contingency values must be non-negative");

  const Series row_sum = obs.sum(1, true);
  const Series col_sum = obs.sum(0, true);
  const double total = scalar_(obs.sum());
  PLUSS_CHECK(total > kEps, std::string(op) + ": total count must be > 0");

  const Series expv = row_sum.matmul(col_sum).div(total).maximum(kEps);
  const double stat = scalar_(obs.sub(expv).mul(obs.sub(expv)).div(expv).sum());
  const double dof = static_cast<double>((r - 1) * (c - 1));
  const double pvalue = clamp01_(1.0 - chisq_cdf_(stat, static_cast<std::int64_t>(std::llround(dof)), out_cfg));
  const double critical = chisq_quantile_(1.0 - alpha, static_cast<std::int64_t>(std::llround(dof)), out_cfg);

  ChiIndepResult out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = stat > critical;
  out.dof = dof;
  out.rows = r;
  out.cols = c;
  out.expected = expv;
  return out;
}

} // namespace pluss::subsp::stats::inference::hypothesis
