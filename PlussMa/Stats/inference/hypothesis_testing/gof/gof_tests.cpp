#include "pluss/subsp/stats/inference/hypothesis_testing.h"

#include "CodeM/core/Errors.h"
#include "pluss/tils/kit/scalar_cast.h"
#include "pluss/subsp/stats/tails.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/stats/distr.h"
#include "jb_tables.h"

#include <algorithm>
#include <array>
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
  return pluss::tils::to_scalar<double>(x.to(ScalarType::Float64));
}

Series finite_values_sorted_(const Series& x, const char* op, Config cfg) {
  const auto out_cfg = f64_cfg_(cfg);
  const Series raw = as_vector_(x, op);
  const Series base = raw.to(out_cfg.platform, out_cfg.dtype).reshape({raw.numel()});
  return base.masked_select(base.isfinite()).sort(0, false, false);
}

double normal_cdf_(double x, double mean, double stddev, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::tils::to_scalar<double>(pluss::subsp::stats::distr::Normal{mean, stddev}.cdf(q)));
}

double chisq_cdf_(double x, std::int64_t dof, Config cfg) {
  const Series q = pluss::make_series(std::vector<double>{x}, {1}, f64_cfg_(cfg));
  return clamp01_(pluss::tils::to_scalar<double>(pluss::subsp::stats::distr::ChiSquare{dof}.cdf(q)));
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

// ---- exact Kolmogorov-Smirnov distributions ------------------------------------------------------------------------------------

// One sample, two-sided: P(D_n < d) by the matrix-power method of Marsaglia, Tsang and Wang (2003), carrying a decimal exponent so
// that the repeated squaring cannot overflow.
void ks_matrix_multiply_(const std::vector<double>& a, const std::vector<double>& b, std::vector<double>& c, int m) {
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m; ++j) {
      double s = 0.0;
      for (int k = 0; k < m; ++k) {
        s += a[static_cast<std::size_t>(i * m + k)] * b[static_cast<std::size_t>(k * m + j)];
      }
      c[static_cast<std::size_t>(i * m + j)] = s;
    }
  }
}

void ks_matrix_power_(const std::vector<double>& a, int ea, std::vector<double>& v, int& ev, int m, int n) {
  if (n == 1) {
    v = a;
    ev = ea;
    return;
  }
  ks_matrix_power_(a, ea, v, ev, m, n / 2);
  std::vector<double> b(static_cast<std::size_t>(m * m));
  ks_matrix_multiply_(v, v, b, m);
  int eb = 2 * ev;
  if (n % 2 == 0) {
    v = b;
    ev = eb;
  } else {
    ks_matrix_multiply_(a, b, v, m);
    ev = ea + eb;
  }
  if (v[static_cast<std::size_t>((m / 2) * m + (m / 2))] > 1e140) {
    for (auto& x : v) {
      x *= 1e-140;
    }
    ev += 140;
  }
}

double ks_cdf_exact_one_sample_(int n, double d) {
  const int k = static_cast<int>(n * d) + 1;
  const int m = 2 * k - 1;
  const double h = k - n * d;
  std::vector<double> hm(static_cast<std::size_t>(m * m));
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m; ++j) {
      hm[static_cast<std::size_t>(i * m + j)] = (i - j + 1 < 0) ? 0.0 : 1.0;
    }
  }
  for (int i = 0; i < m; ++i) {
    hm[static_cast<std::size_t>(i * m)] -= std::pow(h, i + 1);
    hm[static_cast<std::size_t>((m - 1) * m + i)] -= std::pow(h, m - i);
  }
  hm[static_cast<std::size_t>((m - 1) * m)] += (2 * h - 1 > 0) ? std::pow(2 * h - 1, m) : 0.0;
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m; ++j) {
      if (i - j + 1 > 0) {
        for (int g = 1; g <= i - j + 1; ++g) {
          hm[static_cast<std::size_t>(i * m + j)] /= g;
        }
      }
    }
  }
  std::vector<double> q;
  int eq = 0;
  ks_matrix_power_(hm, 0, q, eq, m, n);
  double s = q[static_cast<std::size_t>((k - 1) * m + k - 1)];
  for (int i = 1; i <= n; ++i) {
    s = s * i / n;
    if (s < 1e-140) {
      s *= 1e140;
      eq -= 140;
    }
  }
  return s * std::pow(10.0, eq);
}

// One sample, one-sided: P(D+_n >= d) (Birnbaum and Tingey 1951), summed in logarithms.
double ks_one_sided_exact_one_sample_(int n, double d) {
  if (d <= 0.0) {
    return 1.0;
  }
  if (d >= 1.0) {
    return 0.0;
  }
  double sum = 0.0;
  const int jmax = static_cast<int>(std::floor(n * (1.0 - d)));
  for (int j = 0; j <= jmax; ++j) {
    const double a = 1.0 - d - static_cast<double>(j) / n;
    const double b = d + static_cast<double>(j) / n;
    if (a <= 0.0) {
      continue;
    }
    const double logterm = std::lgamma(n + 1.0) - std::lgamma(j + 1.0) - std::lgamma(n - j + 1.0) + (n - j) * std::log(a) + (j - 1) * std::log(b);
    sum += std::exp(logterm);
  }
  return clamp01_(d * sum);
}

// Two samples: P(D >= d) by counting the lattice paths from (0,0) to (nx,ny) that stay strictly inside the band |i/nx - j/ny| < d
// (two-sided) or that never reach i/nx - j/ny >= d (side = +1) or j/ny - i/nx >= d (side = -1). Differences are compared as integers
// i*ny - j*nx against round(d*nx*ny), which is exact because the statistic is a multiple of 1/(nx*ny).
double ks_two_sample_exact_p_(std::int64_t nx, std::int64_t ny, double d, int side) {
  if (d <= 0.0) {
    return 1.0;
  }
  const std::int64_t limit = static_cast<std::int64_t>(std::llround(d * static_cast<double>(nx) * static_cast<double>(ny)));
  std::vector<double> prev(static_cast<std::size_t>(ny + 1), 0.0), cur(static_cast<std::size_t>(ny + 1), 0.0);
  auto inside = [&](std::int64_t i, std::int64_t j) {
    const std::int64_t diff = i * ny - j * nx;
    if (side == 0) {
      return std::llabs(diff) < limit;
    }
    return side > 0 ? diff < limit : -diff < limit;
  };
  for (std::int64_t i = 0; i <= nx; ++i) {
    for (std::int64_t j = 0; j <= ny; ++j) {
      double ways = 0.0;
      if (i == 0 && j == 0) {
        ways = 1.0;
      } else {
        if (i > 0) {
          ways += prev[static_cast<std::size_t>(j)];
        }
        if (j > 0) {
          ways += cur[static_cast<std::size_t>(j - 1)];
        }
      }
      cur[static_cast<std::size_t>(j)] = inside(i, j) ? ways : 0.0;
    }
    std::swap(prev, cur);
  }
  const double total = std::exp(std::lgamma(nx + ny + 1.0) - std::lgamma(nx + 1.0) - std::lgamma(ny + 1.0));
  return clamp01_(1.0 - prev[static_cast<std::size_t>(ny)] / total);
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


// Finite-sample p-value of the Jarque-Bera statistic. The chi-squared(2) limit is a poor guide below a few hundred
// observations (at n = 25 it reports about 0.56 where the true tail probability is 0.37), so the statistic is looked up in the
// simulated null distribution of jb_tables.h: critical values at fixed upper-tail levels, interpolated in log(n) between
// the tabulated sample sizes and in log(p) between levels. Beyond the last sample size the limit is used; beyond the
// smallest tabulated level the tail continues with the chi-squared(2) decay rate.
double jb_pvalue_(double jb, std::int64_t n) {
  using namespace jb_tables;
  if (n > kN[kSizes - 1]) {
    return pluss::subsp::stats::tails::chi2_sf(jb, 2.0);
  }
  std::array<double, kLevels> c{};
  const double ln = std::log(static_cast<double>(n));
  int i = 0;
  while (i + 2 < kSizes && static_cast<double>(kN[i + 1]) < static_cast<double>(n)) {
    ++i;
  }
  const double w = std::clamp((ln - std::log(static_cast<double>(kN[i]))) / (std::log(static_cast<double>(kN[i + 1])) - std::log(static_cast<double>(kN[i]))), 0.0, 1.0);
  for (int l = 0; l < kLevels; ++l) {
    c[static_cast<std::size_t>(l)] = (1.0 - w) * kCritical[i][l] + w * kCritical[i + 1][l];
  }
  if (jb <= c[0]) {
    return 1.0 - (1.0 - kQ[0]) * std::max(0.0, jb) / std::max(kEps, c[0]);
  }
  if (jb >= c[kLevels - 1]) {
    return kQ[kLevels - 1] * std::exp(-0.5 * (jb - c[kLevels - 1]));
  }
  int l = 0;
  while (l + 1 < kLevels - 1 && jb >= c[static_cast<std::size_t>(l + 1)]) {
    ++l;
  }
  const double lo = c[static_cast<std::size_t>(l)];
  const double hi = c[static_cast<std::size_t>(l + 1)];
  const double t = hi > lo ? (jb - lo) / (hi - lo) : 1.0;
  return std::exp((1.0 - t) * std::log(kQ[l]) + t * std::log(kQ[l + 1]));
}

double jb_critical_(double alpha, std::int64_t n, const Config& cfg) {
  if (n > jb_tables::kN[jb_tables::kSizes - 1]) {
    return chisq_quantile_(1.0 - alpha, 2, cfg);
  }
  double lo = 0.0;
  double hi = 1.0;
  while (jb_pvalue_(hi, n) > alpha && hi < 1e6) {
    hi *= 2.0;
  }
  for (int it = 0; it < 100; ++it) {
    const double mid = 0.5 * (lo + hi);
    if (jb_pvalue_(mid, n) > alpha) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
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
  // Exact distributions for samples of up to 140 observations (as for the usual statistical packages), the Kolmogorov series otherwise.
  const bool exact = options.method == PValueMethod::Exact || (options.method == PValueMethod::Auto && n <= 140);
  double pvalue = 0.0;
  if (options.tail == Tail::Both) {
    pvalue = exact ? clamp01_(1.0 - ks_cdf_exact_one_sample_(static_cast<int>(n), stat)) : ks_two_sided_pvalue_(n_eff, stat);
  } else {
    pvalue = exact ? ks_one_sided_exact_one_sample_(static_cast<int>(n), stat) : ks_one_sided_pvalue_(n_eff, stat);
  }
  const double critical = (options.tail == Tail::Both)
                              ? ks_critical_two_sided_(n_eff, options.alpha)
                              : ks_critical_one_sided_(n_eff, options.alpha);

  KSTestResult out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = pvalue < options.alpha;
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

  // Each empirical distribution function is evaluated as the fraction of its own sample that is <= the point, so repeated
  // values are counted together (an index-based i/n would understate the step at a tie and overstate the distance).
  const Series fx_x = xvals.reshape({1, nx}).le(xvals.reshape({nx, 1})).to(out_cfg.dtype).sum(1, false).div(static_cast<double>(nx));
  const Series fy_y = yvals.reshape({1, ny}).le(yvals.reshape({ny, 1})).to(out_cfg.dtype).sum(1, false).div(static_cast<double>(ny));
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
  // Exact distribution while nx * ny < 10000, the Kolmogorov series otherwise.
  const bool exact = options.method == PValueMethod::Exact ||
                     (options.method == PValueMethod::Auto && nx * ny < 10000);
  double pvalue = 0.0;
  if (exact) {
    const int side = options.tail == Tail::Both ? 0 : (options.tail == Tail::Right ? 1 : -1);
    pvalue = ks_two_sample_exact_p_(nx, ny, stat, side);
  } else {
    pvalue = (options.tail == Tail::Both) ? ks_two_sided_pvalue_(n_eff, stat) : ks_one_sided_pvalue_(n_eff, stat);
  }
  const double critical = (options.tail == Tail::Both)
                              ? ks_critical_two_sided_(n_eff, options.alpha)
                              : ks_critical_one_sided_(n_eff, options.alpha);

  KSTest2Result out;
  out.stat = stat;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = pvalue < options.alpha;
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

  const double pvalue = jb_pvalue_(jb, n);
  const double critical = jb_critical_(alpha, n, out_cfg);

  JBTestResult out;
  out.stat = jb;
  out.pvalue = clamp01_(pvalue);
  out.critical = critical;
  out.reject = pvalue < alpha;
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
  const double pvalue = clamp01_(pluss::subsp::stats::tails::chi2_sf(stat, std::round(dof)));
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
  const double pvalue = clamp01_(pluss::subsp::stats::tails::chi2_sf(stat, std::round(dof)));
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
