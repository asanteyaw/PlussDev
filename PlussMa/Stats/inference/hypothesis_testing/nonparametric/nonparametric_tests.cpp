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

std::vector<double> finite_values_(const Series& x, const char* op) {
  const Series xv = as_vector_(x, op).to(pluss::pCPU, ScalarType::Float64);
  const auto raw = xv.to_vec<double>();
  std::vector<double> out;
  out.reserve(raw.size());
  for (double v : raw) {
    if (std::isfinite(v)) {
      out.push_back(v);
    }
  }
  return out;
}

std::vector<double> finite_diffs_(const Series& x, double center, const char* op) {
  const auto xv = finite_values_(x, op);
  std::vector<double> out;
  out.reserve(xv.size());
  for (double v : xv) {
    out.push_back(v - center);
  }
  return out;
}

std::vector<double> finite_paired_diffs_(const Series& x, const Series& y, const char* op) {
  const Series xv = as_vector_(x, op).to(pluss::pCPU, ScalarType::Float64);
  const Series yv = as_vector_(y, op).to(pluss::pCPU, ScalarType::Float64);
  PLUSS_CHECK(xv.numel() == yv.numel(), std::string(op) + ": paired inputs must have same length");
  const auto xa = xv.to_vec<double>();
  const auto ya = yv.to_vec<double>();
  std::vector<double> out;
  out.reserve(xa.size());
  for (std::size_t i = 0; i < xa.size(); ++i) {
    if (std::isfinite(xa[i]) && std::isfinite(ya[i])) {
      out.push_back(xa[i] - ya[i]);
    }
  }
  return out;
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

double p_from_cdf_(double cdf, Tail tail) {
  const double c = clamp01_(cdf);
  switch (tail) {
    case Tail::Both:
      return clamp01_(2.0 * std::min(c, 1.0 - c));
    case Tail::Left:
      return c;
    case Tail::Right:
      return 1.0 - c;
  }
  return 1.0;
}

struct RankedResult {
  std::vector<double> ranks;
  std::vector<std::int64_t> tie_sizes;
};

RankedResult average_ranks_(const std::vector<double>& values) {
  const std::size_t n = values.size();
  std::vector<std::size_t> idx(n);
  std::iota(idx.begin(), idx.end(), 0);
  std::stable_sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) {
    return values[a] < values[b];
  });

  RankedResult out;
  out.ranks.assign(n, 0.0);
  std::size_t i = 0;
  while (i < n) {
    std::size_t j = i + 1;
    while (j < n && values[idx[j]] == values[idx[i]]) {
      ++j;
    }
    const double avg_rank = 0.5 * static_cast<double>((i + 1) + j);
    for (std::size_t k = i; k < j; ++k) {
      out.ranks[idx[k]] = avg_rank;
    }
    const std::int64_t t = static_cast<std::int64_t>(j - i);
    if (t > 1) {
      out.tie_sizes.push_back(t);
    }
    i = j;
  }
  return out;
}

double continuity_corrected_z_(double stat, double mean, double sd, Tail tail) {
  if (sd <= 0.0) {
    return 0.0;
  }
  if (tail == Tail::Both) {
    const double delta = stat - mean;
    const double cc = (delta > 0.0) ? 0.5 : ((delta < 0.0) ? -0.5 : 0.0);
    return (stat - mean - cc) / sd;
  }
  if (tail == Tail::Right) {
    return (stat - mean - 0.5) / sd;
  }
  return (stat - mean + 0.5) / sd;
}

double binom_cdf_half_(std::int64_t n, std::int64_t k) {
  if (k < 0) {
    return 0.0;
  }
  if (k >= n) {
    return 1.0;
  }
  const double logp0 = -static_cast<double>(n) * std::log(2.0);
  double term = std::exp(logp0);
  double cdf = term;
  for (std::int64_t i = 0; i < k; ++i) {
    term *= static_cast<double>(n - i) / static_cast<double>(i + 1);
    cdf += term;
  }
  return clamp01_(cdf);
}

SignTestResult signtest_from_diffs_(const std::vector<double>& diffs,
                                    const TestOptions& options,
                                    Config cfg,
                                    const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  check_alpha_(options.alpha, op);

  std::int64_t pos = 0;
  std::int64_t neg = 0;
  for (double d : diffs) {
    if (!std::isfinite(d) || std::abs(d) <= kEps) {
      continue;
    }
    if (d > 0.0) {
      ++pos;
    } else {
      ++neg;
    }
  }
  const std::int64_t n = pos + neg;
  PLUSS_CHECK(n > 0, std::string(op) + ": no nonzero finite differences");

  const double mean = 0.5 * static_cast<double>(n);
  const double sd = 0.5 * std::sqrt(static_cast<double>(n));
  const double z = (static_cast<double>(pos) - mean) / std::max(sd, std::sqrt(kEps));

  const double cdf_at_pos = binom_cdf_half_(n, pos);
  const double cdf_before_pos = binom_cdf_half_(n, pos - 1);

  double pvalue = 1.0;
  if (options.tail == Tail::Right) {
    pvalue = 1.0 - cdf_before_pos;
  } else if (options.tail == Tail::Left) {
    pvalue = cdf_at_pos;
  } else {
    pvalue = 2.0 * std::min(cdf_at_pos, 1.0 - cdf_before_pos);
  }
  pvalue = clamp01_(pvalue);

  double critical = 0.0;
  if (options.tail == Tail::Both) {
    critical = normal_quantile_(1.0 - options.alpha * 0.5, out_cfg);
  } else if (options.tail == Tail::Right) {
    critical = normal_quantile_(1.0 - options.alpha, out_cfg);
  } else {
    critical = normal_quantile_(options.alpha, out_cfg);
  }

  SignTestResult out;
  out.stat = z;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = (pvalue < options.alpha);
  out.positives = pos;
  out.negatives = neg;
  out.effective_n = n;
  return out;
}

// Exact null distributions for the rank tests. Ranks are doubled so that tied (half-integer) ranks stay integers.
std::vector<std::int64_t> doubled_ranks_(const std::vector<double>& ranks) {
  std::vector<std::int64_t> out;
  out.reserve(ranks.size());
  for (double r : ranks) {
    out.push_back(static_cast<std::int64_t>(std::llround(2.0 * r)));
  }
  return out;
}

// Tail probabilities P(S <= s) and P(S >= s) of a discrete distribution on the doubled-integer support.
std::pair<double, double> pmf_tails_(const std::vector<double>& pmf, std::int64_t s) {
  double left = 0.0;
  double right = 0.0;
  for (std::int64_t v = 0; v < static_cast<std::int64_t>(pmf.size()); ++v) {
    if (v <= s) {
      left += pmf[static_cast<std::size_t>(v)];
    }
    if (v >= s) {
      right += pmf[static_cast<std::size_t>(v)];
    }
  }
  return {std::min(1.0, left), std::min(1.0, right)};
}

double exact_p_from_tails_(double left, double right, Tail tail) {
  switch (tail) {
    case Tail::Left:
      return clamp01_(left);
    case Tail::Right:
      return clamp01_(right);
    case Tail::Both:
      return clamp01_(2.0 * std::min(left, right));
  }
  return 1.0;
}

// Signed-rank statistic under H0: each rank enters the sum with probability 1/2.
double signrank_exact_p_(const std::vector<double>& ranks, double wplus, Tail tail) {
  const auto d = doubled_ranks_(ranks);
  std::int64_t total = 0;
  for (auto v : d) {
    total += v;
  }
  std::vector<double> pmf(static_cast<std::size_t>(total + 1), 0.0);
  pmf[0] = 1.0;
  std::int64_t reach = 0;
  for (auto v : d) {
    reach += v;
    for (std::int64_t s = reach; s >= 0; --s) {
      const double shifted = (s >= v) ? pmf[static_cast<std::size_t>(s - v)] : 0.0;
      pmf[static_cast<std::size_t>(s)] = 0.5 * (pmf[static_cast<std::size_t>(s)] + shifted);
    }
  }
  const auto [left, right] = pmf_tails_(pmf, static_cast<std::int64_t>(std::llround(2.0 * wplus)));
  return exact_p_from_tails_(left, right, tail);
}

// Rank-sum statistic under H0: the sample X is a uniformly random subset of size nx of the pooled ranks.
double ranksum_exact_p_(const std::vector<double>& ranks, std::int64_t nx, double rank_sum_x, Tail tail) {
  const auto d = doubled_ranks_(ranks);
  std::int64_t total = 0;
  for (auto v : d) {
    total += v;
  }
  std::vector<std::vector<double>> ways(static_cast<std::size_t>(nx + 1), std::vector<double>(static_cast<std::size_t>(total + 1), 0.0));
  ways[0][0] = 1.0;
  std::int64_t seen = 0;
  for (std::size_t i = 0; i < d.size(); ++i) {
    seen += d[i];
    const std::int64_t kmax = std::min<std::int64_t>(static_cast<std::int64_t>(i) + 1, nx);
    for (std::int64_t k = kmax; k >= 1; --k) {
      for (std::int64_t s = seen; s >= d[i]; --s) {
        ways[static_cast<std::size_t>(k)][static_cast<std::size_t>(s)] += ways[static_cast<std::size_t>(k - 1)][static_cast<std::size_t>(s - d[i])];
      }
    }
  }
  double count = 0.0;
  for (double w : ways[static_cast<std::size_t>(nx)]) {
    count += w;
  }
  std::vector<double> pmf(ways[static_cast<std::size_t>(nx)].size());
  for (std::size_t s = 0; s < pmf.size(); ++s) {
    pmf[s] = ways[static_cast<std::size_t>(nx)][s] / count;
  }
  const auto [left, right] = pmf_tails_(pmf, static_cast<std::int64_t>(std::llround(2.0 * rank_sum_x)));
  return exact_p_from_tails_(left, right, tail);
}

SignRankResult signrank_from_diffs_(const std::vector<double>& diffs,
                                    const TestOptions& options,
                                    Config cfg,
                                    const char* op) {
  const auto out_cfg = f64_cfg_(cfg);
  check_alpha_(options.alpha, op);

  std::vector<double> absdiff;
  std::vector<bool> positive;
  absdiff.reserve(diffs.size());
  positive.reserve(diffs.size());
  for (double d : diffs) {
    if (!std::isfinite(d) || std::abs(d) <= kEps) {
      continue;
    }
    absdiff.push_back(std::abs(d));
    positive.push_back(d > 0.0);
  }

  const std::int64_t n = static_cast<std::int64_t>(absdiff.size());
  PLUSS_CHECK(n > 0, std::string(op) + ": no nonzero finite differences");

  const auto rank_info = average_ranks_(absdiff);
  const auto& ranks = rank_info.ranks;

  double wplus = 0.0;
  double total_rank = 0.0;
  double sumsq_rank = 0.0;
  for (std::size_t i = 0; i < ranks.size(); ++i) {
    total_rank += ranks[i];
    sumsq_rank += ranks[i] * ranks[i];
    if (positive[i]) {
      wplus += ranks[i];
    }
  }
  const double wminus = total_rank - wplus;

  const double mean = 0.5 * total_rank;
  const double var = 0.25 * sumsq_rank;
  const double sd = std::sqrt(std::max(kEps, var));
  const double z = continuity_corrected_z_(wplus, mean, sd, options.tail);
  // Exact distribution for small samples (n <= 15 nonzero differences), the normal approximation otherwise.
  const bool exact = options.method == PValueMethod::Exact || (options.method == PValueMethod::Auto && n <= 15);
  const double pvalue = exact ? signrank_exact_p_(ranks, wplus, options.tail)
                              : p_from_cdf_(normal_cdf_(z, out_cfg), options.tail);

  double critical = 0.0;
  if (options.tail == Tail::Both) {
    critical = normal_quantile_(1.0 - options.alpha * 0.5, out_cfg);
  } else if (options.tail == Tail::Right) {
    critical = normal_quantile_(1.0 - options.alpha, out_cfg);
  } else {
    critical = normal_quantile_(options.alpha, out_cfg);
  }
  const bool reject = pvalue < options.alpha;

  SignRankResult out;
  out.stat = z;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.wplus = wplus;
  out.wminus = wminus;
  out.effective_n = n;
  return out;
}

} // namespace

RankSumResult ranksum(const Series& x, const Series& y, const TestOptions& options, Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::ranksum";
  check_alpha_(options.alpha, op);
  const auto out_cfg = f64_cfg_(cfg);

  const auto xvals = finite_values_(x, op);
  const auto yvals = finite_values_(y, op);
  const std::int64_t nx = static_cast<std::int64_t>(xvals.size());
  const std::int64_t ny = static_cast<std::int64_t>(yvals.size());
  PLUSS_CHECK(nx > 0 && ny > 0, std::string(op) + ": requires at least one finite observation in each sample");

  std::vector<double> all;
  all.reserve(static_cast<std::size_t>(nx + ny));
  std::vector<bool> is_x;
  is_x.reserve(static_cast<std::size_t>(nx + ny));
  for (double v : xvals) {
    all.push_back(v);
    is_x.push_back(true);
  }
  for (double v : yvals) {
    all.push_back(v);
    is_x.push_back(false);
  }

  const auto rank_info = average_ranks_(all);
  const auto& ranks = rank_info.ranks;

  double rx = 0.0;
  for (std::size_t i = 0; i < ranks.size(); ++i) {
    if (is_x[i]) {
      rx += ranks[i];
    }
  }

  const double nxd = static_cast<double>(nx);
  const double nyd = static_cast<double>(ny);
  const double n = static_cast<double>(nx + ny);
  const double u = rx - (nxd * (nxd + 1.0) * 0.5);
  const double mean_u = 0.5 * nxd * nyd;

  double tie_term = 0.0;
  for (std::int64_t t : rank_info.tie_sizes) {
    const double td = static_cast<double>(t);
    tie_term += td * td * td - td;
  }
  double var_u = nxd * nyd * ((n + 1.0) - tie_term / std::max(1.0, n * (n - 1.0))) / 12.0;

  double z = 0.0;
  if (var_u > kEps) {
    z = continuity_corrected_z_(u, mean_u, std::sqrt(var_u), options.tail);
  }
  // Exact distribution for small samples (the smaller sample under 10 and fewer than 20 observations in all), the normal
  // approximation otherwise.
  const bool exact = options.method == PValueMethod::Exact ||
                     (options.method == PValueMethod::Auto && std::min(nx, ny) < 10 && (nx + ny) < 20);
  const double pvalue = exact ? ranksum_exact_p_(ranks, nx, rx, options.tail)
                              : p_from_cdf_(normal_cdf_(z, out_cfg), options.tail);

  double critical = 0.0;
  if (options.tail == Tail::Both) {
    critical = normal_quantile_(1.0 - options.alpha * 0.5, out_cfg);
  } else if (options.tail == Tail::Right) {
    critical = normal_quantile_(1.0 - options.alpha, out_cfg);
  } else {
    critical = normal_quantile_(options.alpha, out_cfg);
  }
  const bool reject = pvalue < options.alpha;

  RankSumResult out;
  out.stat = z;
  out.pvalue = pvalue;
  out.critical = critical;
  out.reject = reject;
  out.rank_sum_x = rx;
  out.u_stat = u;
  out.effective_nx = nx;
  out.effective_ny = ny;
  return out;
}

SignRankResult signrank(const Series& x,
                        double median0,
                        const TestOptions& options,
                        Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::signrank";
  const auto diffs = finite_diffs_(x, median0, op);
  return signrank_from_diffs_(diffs, options, cfg, op);
}

SignRankResult signrank(const Series& x,
                        const Series& y,
                        const TestOptions& options,
                        Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::signrank";
  const auto diffs = finite_paired_diffs_(x, y, op);
  return signrank_from_diffs_(diffs, options, cfg, op);
}

SignTestResult signtest(const Series& x,
                        double median0,
                        const TestOptions& options,
                        Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::signtest";
  const auto diffs = finite_diffs_(x, median0, op);
  return signtest_from_diffs_(diffs, options, cfg, op);
}

SignTestResult signtest(const Series& x,
                        const Series& y,
                        const TestOptions& options,
                        Config cfg) {
  const char* op = "subsp::stats::inference::hypothesis::signtest";
  const auto diffs = finite_paired_diffs_(x, y, op);
  return signtest_from_diffs_(diffs, options, cfg, op);
}

} // namespace pluss::subsp::stats::inference::hypothesis
