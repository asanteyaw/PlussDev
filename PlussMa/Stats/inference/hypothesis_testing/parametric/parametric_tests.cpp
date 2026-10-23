#include "pluss/subsp/stats/inference/hypothesis_testing.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/distr.h"
#include "pluss/subsp/stats/tails.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
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

double scalar_(const Series& x) {
  return x.to(ScalarType::Float64).reshape({1}).item<double>();
}

Series ci_series_(double lo, double hi, Config cfg) {
  return pluss::make_series(std::vector<double>{lo, hi}, {2}, f64_cfg_(cfg));
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

Series finite_values_(const Series& x, const char* op, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series base = as_vector_(x, op);
  const Series xv = base.to(out_cfg.platform, out_cfg.dtype).reshape({base.numel()});
  const Series mask = xv.isfinite();
  return xv.masked_select(mask);
}

Series finite_paired_diff_(const Series& x, const Series& y, const char* op, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series xb = as_vector_(x, op);
  const Series yb = as_vector_(y, op);
  const Series xv = xb.to(out_cfg.platform, out_cfg.dtype).reshape({xb.numel()});
  const Series yv = yb.to(out_cfg.platform, out_cfg.dtype).reshape({yb.numel()});
  PLUSS_CHECK(xv.numel() == yv.numel(), std::string(op) + ": paired inputs must have same length");
  const Series mask = xv.isfinite().logical_and(yv.isfinite());
  return xv.sub(yv).masked_select(mask);
}

struct SampleStats {
  std::int64_t n = 0;
  double mean = 0.0;
  double var = 0.0; // unbiased sample variance
};

SampleStats stats_(const Series& x, const char* op, bool require_two = true) {
  const std::int64_t n = x.numel();
  PLUSS_CHECK(n > 0, std::string(op) + ": no finite observations");
  if (require_two) {
    PLUSS_CHECK(n >= 2, std::string(op) + ": requires at least two finite observations");
  }
  const double mean = scalar_(x.mean());
  const Series centered = x.sub(mean);
  const double sumsq = scalar_(centered.mul(centered).sum());
  const double var = (n >= 2) ? (sumsq / static_cast<double>(n - 1)) : 0.0;
  return SampleStats{.n = n, .mean = mean, .var = var};
}

double normal_cdf_(double z, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{z}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::Normal{}.cdf(q).item<double>());
}

double normal_quantile_(double p, Config cfg) {
  if (p <= 0.0) {
    return -std::numeric_limits<double>::infinity();
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = -12.0;
  double hi = 12.0;
  for (std::int64_t i = 0; i < 120; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (normal_cdf_(mid, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double t_cdf_(double x, double dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::StudentT{dof, 0.0, 1.0}.cdf(q).item<double>());
}

double t_quantile_(double p, double dof, Config cfg) {
  if (p <= 0.0) {
    return -std::numeric_limits<double>::infinity();
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = -128.0;
  double hi = 128.0;
  for (std::int64_t i = 0; i < 140; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (t_cdf_(mid, dof, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double chisq_cdf_(double x, double dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::ChiSquare{static_cast<std::int64_t>(std::llround(dof))}.cdf(q).item<double>());
}

double chisq_quantile_(double p, double dof, Config cfg) {
  if (p <= 0.0) {
    return 0.0;
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = 0.0;
  double hi = std::max(1.0, dof + 20.0 * std::sqrt(std::max(dof, 1.0)));
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

double f_cdf_(double x, double dof1, double dof2, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::subsp::stats::distr::FDistribution{dof1, dof2}.cdf(q).item<double>());
}

double f_quantile_(double p, double dof1, double dof2, Config cfg) {
  if (p <= 0.0) {
    return 0.0;
  }
  if (p >= 1.0) {
    return std::numeric_limits<double>::infinity();
  }
  double lo = 0.0;
  double hi = 1.0;
  for (std::int64_t i = 0; i < 120 && f_cdf_(hi, dof1, dof2, cfg) < p; ++i) {
    hi *= 2.0;
  }
  for (std::int64_t i = 0; i < 150; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (f_cdf_(mid, dof1, dof2, cfg) < p) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

// p-value from the two tail probabilities of the statistic, each evaluated directly so that extreme statistics keep a nonzero p-value.
double p_from_tails_(double lower, double upper, Tail tail) {
  switch (tail) {
    case Tail::Both:
      return clamp01_(2.0 * std::min(lower, upper));
    case Tail::Left:
      return clamp01_(lower);
    case Tail::Right:
      return clamp01_(upper);
  }
  return 1.0;
}

// Tail probabilities of a Student t statistic: {P(T <= t), P(T >= t)}.
std::pair<double, double> t_tails_(double t, double dof) {
  const double half = 0.5 * pluss::subsp::stats::tails::student_t_two_sided(std::abs(t), dof);
  return t >= 0.0 ? std::make_pair(1.0 - half, half) : std::make_pair(half, 1.0 - half);
}

void check_alpha_(double alpha, const char* op) {
  PLUSS_CHECK(std::isfinite(alpha) && alpha > 0.0 && alpha < 1.0,
              std::string(op) + ": alpha must be in (0,1)");
}

} // namespace

TTestResult ttest(const Series& x, double mean0, const TestOptions& options, Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::ttest";
  check_alpha_(options.alpha, op);
  const auto out_cfg = f64_cfg_(cfg);
  const Series values = finite_values_(x, op, cfg);
  const auto st = stats_(values, op, true);
  PLUSS_CHECK(st.var > 0.0, std::string(op) + ": sample variance must be > 0");

  const double se = std::sqrt(st.var / static_cast<double>(st.n));
  const double dof = static_cast<double>(st.n - 1);
  const double stat = (st.mean - mean0) / std::max(se, std::sqrt(kEps));
  const auto tt = t_tails_(stat, dof);
  const double pvalue = p_from_tails_(tt.first, tt.second, options.tail);

  double critical = 0.0;
  bool reject = false;
  double ci_lo = -std::numeric_limits<double>::infinity();
  double ci_hi = std::numeric_limits<double>::infinity();
  if (options.tail == Tail::Both) {
    critical = t_quantile_(1.0 - options.alpha * 0.5, dof, out_cfg);
    reject = std::abs(stat) > critical;
    ci_lo = st.mean - critical * se;
    ci_hi = st.mean + critical * se;
  } else if (options.tail == Tail::Right) {
    critical = t_quantile_(1.0 - options.alpha, dof, out_cfg);
    reject = stat > critical;
    ci_lo = st.mean - critical * se;
  } else {
    critical = t_quantile_(options.alpha, dof, out_cfg);
    reject = stat < critical;
    ci_hi = st.mean + t_quantile_(1.0 - options.alpha, dof, out_cfg) * se;
  }

  TTestResult out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.dof = dof;
  out.mean = st.mean;
  out.null_mean = mean0;
  out.stderr = se;
  out.ci = ci_series_(ci_lo, ci_hi, out_cfg);
  out.effective_n = st.n;
  return out;
}

TTestResult ttest_paired(const Series& x, const Series& y, const TestOptions& options, Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::ttest_paired";
  check_alpha_(options.alpha, op);
  const Series diff = finite_paired_diff_(x, y, op, cfg);
  return ttest(diff, 0.0, options, cfg);
}

TTest2Result ttest2(const Series& x, const Series& y, const TTest2Options& options, Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::ttest2";
  check_alpha_(options.alpha, op);
  const auto out_cfg = f64_cfg_(cfg);
  const Series xvals = finite_values_(x, op, cfg);
  const Series yvals = finite_values_(y, op, cfg);
  const auto sx = stats_(xvals, op, true);
  const auto sy = stats_(yvals, op, true);
  PLUSS_CHECK(sx.var > 0.0 && sy.var > 0.0, std::string(op) + ": sample variances must be > 0");

  const double nx = static_cast<double>(sx.n);
  const double ny = static_cast<double>(sy.n);
  const double diff = sx.mean - sy.mean;

  double se = 0.0;
  double dof = 0.0;
  if (options.equal_variance) {
    const double pooled_var = ((nx - 1.0) * sx.var + (ny - 1.0) * sy.var) / (nx + ny - 2.0);
    se = std::sqrt(std::max(kEps, pooled_var * (1.0 / nx + 1.0 / ny)));
    dof = nx + ny - 2.0;
  } else {
    const double vx = sx.var / nx;
    const double vy = sy.var / ny;
    se = std::sqrt(std::max(kEps, vx + vy));
    const double denom = (vx * vx) / std::max(1.0, nx - 1.0) + (vy * vy) / std::max(1.0, ny - 1.0);
    dof = (vx + vy) * (vx + vy) / std::max(kEps, denom);
  }

  const double stat = diff / std::max(se, std::sqrt(kEps));
  const auto tt = t_tails_(stat, dof);
  const double pvalue = p_from_tails_(tt.first, tt.second, options.tail);

  double critical = 0.0;
  bool reject = false;
  double ci_lo = -std::numeric_limits<double>::infinity();
  double ci_hi = std::numeric_limits<double>::infinity();
  if (options.tail == Tail::Both) {
    critical = t_quantile_(1.0 - options.alpha * 0.5, dof, out_cfg);
    reject = std::abs(stat) > critical;
    ci_lo = diff - critical * se;
    ci_hi = diff + critical * se;
  } else if (options.tail == Tail::Right) {
    critical = t_quantile_(1.0 - options.alpha, dof, out_cfg);
    reject = stat > critical;
    ci_lo = diff - critical * se;
  } else {
    critical = t_quantile_(options.alpha, dof, out_cfg);
    reject = stat < critical;
    ci_hi = diff + t_quantile_(1.0 - options.alpha, dof, out_cfg) * se;
  }

  TTest2Result out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.dof = dof;
  out.mean_x = sx.mean;
  out.mean_y = sy.mean;
  out.stderr = se;
  out.equal_variance = options.equal_variance;
  out.ci = ci_series_(ci_lo, ci_hi, out_cfg);
  out.effective_nx = sx.n;
  out.effective_ny = sy.n;
  return out;
}

ZTestResult ztest(const Series& x,
                  double mean0,
                  double sigma,
                  const TestOptions& options,
                  Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::ztest";
  check_alpha_(options.alpha, op);
  PLUSS_CHECK(std::isfinite(sigma) && sigma > 0.0, std::string(op) + ": sigma must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series values = finite_values_(x, op, cfg);
  const auto st = stats_(values, op, false);

  const double se = sigma / std::sqrt(static_cast<double>(st.n));
  const double stat = (st.mean - mean0) / std::max(se, std::sqrt(kEps));
  const double pvalue = p_from_tails_(pluss::subsp::stats::tails::normal_sf(-stat), pluss::subsp::stats::tails::normal_sf(stat), options.tail);

  double critical = 0.0;
  bool reject = false;
  double ci_lo = -std::numeric_limits<double>::infinity();
  double ci_hi = std::numeric_limits<double>::infinity();
  if (options.tail == Tail::Both) {
    critical = normal_quantile_(1.0 - options.alpha * 0.5, out_cfg);
    reject = std::abs(stat) > critical;
    ci_lo = st.mean - critical * se;
    ci_hi = st.mean + critical * se;
  } else if (options.tail == Tail::Right) {
    critical = normal_quantile_(1.0 - options.alpha, out_cfg);
    reject = stat > critical;
    ci_lo = st.mean - critical * se;
  } else {
    critical = normal_quantile_(options.alpha, out_cfg);
    reject = stat < critical;
    ci_hi = st.mean + normal_quantile_(1.0 - options.alpha, out_cfg) * se;
  }

  ZTestResult out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.mean = st.mean;
  out.null_mean = mean0;
  out.sigma = sigma;
  out.stderr = se;
  out.ci = ci_series_(ci_lo, ci_hi, out_cfg);
  out.effective_n = st.n;
  return out;
}

VarTestResult vartest(const Series& x,
                      double variance0,
                      const TestOptions& options,
                      Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::vartest";
  check_alpha_(options.alpha, op);
  PLUSS_CHECK(std::isfinite(variance0) && variance0 > 0.0, std::string(op) + ": variance0 must be > 0");
  const auto out_cfg = f64_cfg_(cfg);
  const Series values = finite_values_(x, op, cfg);
  const auto st = stats_(values, op, true);

  const double dof = static_cast<double>(st.n - 1);
  const double chi = dof * st.var / variance0;
  const double upper_tail = pluss::subsp::stats::tails::chi2_sf(chi, dof);
  const double pvalue = p_from_tails_(chisq_cdf_(chi, dof, out_cfg), upper_tail, options.tail);

  double critical = 0.0;
  bool reject = false;
  double ci_lo = 0.0;
  double ci_hi = std::numeric_limits<double>::infinity();
  if (options.tail == Tail::Both) {
    const double lo = chisq_quantile_(options.alpha * 0.5, dof, out_cfg);
    const double hi = chisq_quantile_(1.0 - options.alpha * 0.5, dof, out_cfg);
    critical = hi;
    reject = (chi < lo) || (chi > hi);
    ci_lo = dof * st.var / hi;
    ci_hi = dof * st.var / std::max(kEps, lo);
  } else if (options.tail == Tail::Right) {
    critical = chisq_quantile_(1.0 - options.alpha, dof, out_cfg);
    reject = chi > critical;
    ci_lo = dof * st.var / critical;
  } else {
    critical = chisq_quantile_(options.alpha, dof, out_cfg);
    reject = chi < critical;
    ci_hi = dof * st.var / std::max(kEps, critical);
  }

  VarTestResult out;
  out.stat = chi;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.dof = dof;
  out.sample_variance = st.var;
  out.null_variance = variance0;
  out.ci = ci_series_(ci_lo, ci_hi, out_cfg);
  out.effective_n = st.n;
  return out;
}

VarTest2Result vartest2(const Series& x,
                        const Series& y,
                        const TestOptions& options,
                        Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::vartest2";
  check_alpha_(options.alpha, op);
  const auto out_cfg = f64_cfg_(cfg);
  const Series xvals = finite_values_(x, op, cfg);
  const Series yvals = finite_values_(y, op, cfg);
  const auto sx = stats_(xvals, op, true);
  const auto sy = stats_(yvals, op, true);
  PLUSS_CHECK(sx.var > 0.0 && sy.var > 0.0, std::string(op) + ": sample variances must be > 0");

  const double dof1 = static_cast<double>(sx.n - 1);
  const double dof2 = static_cast<double>(sy.n - 1);
  const double fstat = sx.var / sy.var;
  const double upper_tail = pluss::subsp::stats::tails::f_sf(fstat, dof1, dof2);
  const double pvalue = p_from_tails_(f_cdf_(fstat, dof1, dof2, out_cfg), upper_tail, options.tail);

  double critical = 0.0;
  bool reject = false;
  double ci_lo = 0.0;
  double ci_hi = std::numeric_limits<double>::infinity();
  if (options.tail == Tail::Both) {
    const double lo = f_quantile_(options.alpha * 0.5, dof1, dof2, out_cfg);
    const double hi = f_quantile_(1.0 - options.alpha * 0.5, dof1, dof2, out_cfg);
    critical = hi;
    reject = (fstat < lo) || (fstat > hi);
    ci_lo = fstat / hi;
    ci_hi = fstat / std::max(kEps, lo);
  } else if (options.tail == Tail::Right) {
    critical = f_quantile_(1.0 - options.alpha, dof1, dof2, out_cfg);
    reject = fstat > critical;
    ci_lo = fstat / critical;
  } else {
    critical = f_quantile_(options.alpha, dof1, dof2, out_cfg);
    reject = fstat < critical;
    ci_hi = fstat / std::max(kEps, critical);
  }

  VarTest2Result out;
  out.stat = fstat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.dof1 = dof1;
  out.dof2 = dof2;
  out.var_x = sx.var;
  out.var_y = sy.var;
  out.ci = ci_series_(ci_lo, ci_hi, out_cfg);
  out.effective_nx = sx.n;
  out.effective_ny = sy.n;
  return out;
}

Anova1WResult anova_1w(const Series& y,
                       const Series& groups,
                       const Anova1WOptions& options,
                       Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::anova_1w";
  check_alpha_(options.alpha, op);

  const auto out_cfg = f64_cfg_(cfg);
  const Series yv = as_vector_(y, op).to(out_cfg.platform, out_cfg.dtype);
  const Series gv = as_vector_(groups, op).to(out_cfg.platform, out_cfg.dtype);
  PLUSS_CHECK(yv.numel() == gv.numel(), std::string(op) + ": y and groups must have the same length");

  const Series row_ok = yv.isfinite().logical_and(gv.isfinite());
  const std::int64_t n_clean = static_cast<std::int64_t>(std::llround(
      scalar_(row_ok.to(ScalarType::Float64).sum())));
  PLUSS_CHECK(n_clean >= 2, std::string(op) + ": need at least two finite observations");
  const bool all_ok = scalar_(row_ok.all()) > 0.5;
  PLUSS_CHECK(all_ok || options.drop_missing, std::string(op) + ": non-finite values found and drop_missing=false");

  const Series idx = row_ok.nonzero().reshape({n_clean}).to(out_cfg.platform, ScalarType::Int64);
  const Series yc = yv.index_select(0, idx);
  const Series gc = gv.index_select(0, idx);

  const Series g_round = gc.round();
  const bool integer_like = scalar_(gc.sub(g_round).abs().le(1e-9).all()) > 0.5;
  PLUSS_CHECK(integer_like, std::string(op) + ": groups must be integer-coded labels");

  const double gmin = scalar_(g_round.min());
  Series labels = g_round.sub(gmin).to(ScalarType::Int64);
  PLUSS_CHECK(scalar_(labels.ge(0).all()) > 0.5, std::string(op) + ": groups must map to non-negative labels");
  const std::int64_t k_full = static_cast<std::int64_t>(std::llround(scalar_(labels.max()))) + 1;
  PLUSS_CHECK(k_full >= 2, std::string(op) + ": need at least two groups");

  const Series class_ids = Series::arange(0, k_full, 1, ScalarType::Int64).to(out_cfg.platform, ScalarType::Int64);
  const Series mask_full = labels.reshape({n_clean, 1}).eq(class_ids.reshape({1, k_full}));
  const Series counts_full = mask_full.to(out_cfg.dtype).sum(0, false).reshape({k_full});
  const Series active = counts_full.gt(0.0);
  const std::int64_t k = static_cast<std::int64_t>(std::llround(scalar_(active.to(ScalarType::Float64).sum())));
  PLUSS_CHECK(k >= 2, std::string(op) + ": need at least two non-empty groups");

  const Series active_idx = active.nonzero().reshape({k}).to(out_cfg.platform, ScalarType::Int64);
  const Series mask = mask_full.index_select(1, active_idx).to(out_cfg.platform, out_cfg.dtype);
  const Series counts = counts_full.index_select(0, active_idx).to(out_cfg.platform, out_cfg.dtype);

  const Series ycol = yc.reshape({n_clean, 1});
  const Series sums = ycol.mul(mask).sum(0, false).reshape({k});
  const Series means = sums.div(counts.maximum(kEps));

  const double grand_mean = scalar_(yc.mean());
  const double ss_between = scalar_(counts.mul(means.sub(grand_mean).mul(means.sub(grand_mean))).sum());
  const Series resid = ycol.sub(means.reshape({1, k}));
  const double ss_within = scalar_(resid.mul(resid).mul(mask).sum());

  const double dof_between = static_cast<double>(k - 1);
  const double dof_within = static_cast<double>(n_clean - k);
  PLUSS_CHECK(dof_within > 0.0, std::string(op) + ": requires effective_n > number_of_groups");

  const double ms_between = ss_between / std::max(kEps, dof_between);
  const double ms_within = ss_within / std::max(kEps, dof_within);
  const double fstat = ms_between / std::max(kEps, ms_within);
  const double pvalue = clamp01_(pluss::subsp::stats::tails::f_sf(fstat, dof_between, dof_within));
  const double critical = f_quantile_(1.0 - options.alpha, dof_between, dof_within, out_cfg);

  Anova1WResult out;
  out.stat = fstat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = fstat > critical;
  out.dof_between = dof_between;
  out.dof_within = dof_within;
  out.ss_between = ss_between;
  out.ss_within = ss_within;
  out.ms_between = ms_between;
  out.ms_within = ms_within;
  out.groups = k;
  out.effective_n = n_clean;
  out.group_means = means;
  out.group_counts = counts;
  return out;
}

VarTest2Result f_ratio(const Series& x,
                       const Series& y,
                       const TestOptions& options,
                       Config cfg) {
  return vartest2(x, y, options, cfg);
}

} // namespace pluss::subsp::stats::inference::hypothesis
