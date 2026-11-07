#include "pluss/subsp/stats/distr.h"

#include "distr_generic_detail.h"

#include "pluss/tils/auxiliary/scalar_cast.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>

namespace pluss::subsp::stats::distr {

namespace {

using pluss::ScalarType;
using pluss::Series;

constexpr double kInf = std::numeric_limits<double>::infinity();

Series f64_series_(const std::vector<double>& v) {
  return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64);
}

std::vector<double> to_doubles_(const Series& s) {
  return s.to(pluss::pCPU, ScalarType::Float64).contiguous().host<double>().array().data;
}

// The kind-based functions take their parameters as a span or, for per-point parameters, a span of Series.
pluss::msharp::RangeView::Series series_span_(const std::vector<Series>& p) {
  return pluss::msharp::RangeView::Series{p.data(), p.size()};
}

Series dispatch_(const Distribution& d, const Series& x, int which) {
  if (!d.series_params.empty()) {
    const auto sp = series_span_(d.series_params);
    switch (which) {
      case 0: return pdf(d.kind, x, sp);
      case 1: return logpdf(d.kind, x, sp);
      case 2: return cdf(d.kind, x, sp);
      default: return logcdf(d.kind, x, sp);
    }
  }
  const std::span<const double> p(d.params);
  switch (which) {
    case 0: return pdf(d.kind, x, p);
    case 1: return logpdf(d.kind, x, p);
    case 2: return cdf(d.kind, x, p);
    default: return logcdf(d.kind, x, p);
  }
}

void require_scalar_params_(const Distribution& d, const char* op) {
  if (!d.series_params.empty())
    throw std::invalid_argument(std::string("distr::Distribution::") + op +
                                ": not available for an object whose parameters are Series");
}

std::vector<double> cdf_at_(const Distribution& d, const std::vector<double>& x) {
  return to_doubles_(cdf(d.kind, f64_series_(x), std::span<const double>(d.params)));
}

// Smallest x with cdf(x) >= p, for every p at once (one cdf call per bisection step). Continuous: bracket then
// bisection; discrete: the smallest integer.
std::vector<double> quantiles_(const Distribution& d, const std::vector<double>& ps) {
  const bool disc = d.is_discrete();
  const std::size_t n = ps.size();
  std::vector<double> lo(n, -1.0), hi(n, 1.0);
  std::vector<char> bad(n, 0);  // a probability the cdf never reaches (its own accuracy floor): no cut there
  for (int guard = 0; guard < 1100; ++guard) {
    const auto c = cdf_at_(d, lo);
    bool again = false;
    for (std::size_t i = 0; i < n; ++i) if (!bad[i] && c[i] >= ps[i]) {
      if (std::abs(lo[i]) > 1e30) bad[i] = 1; else { lo[i] *= 2.0; again = true; }
    }
    if (disc) for (double v : lo) if (v < -4.0e6) throw std::runtime_error("distr::Distribution: the tails are too heavy to sum over");
    if (!again) break;
  }
  for (int guard = 0; guard < 1100; ++guard) {
    const auto c = cdf_at_(d, hi);
    bool again = false;
    for (std::size_t i = 0; i < n; ++i) if (!bad[i] && c[i] < ps[i]) {
      if (std::abs(hi[i]) > 1e30) bad[i] = 1; else { hi[i] *= 2.0; again = true; }
    }
    if (disc) for (double v : hi) if (v > 4.0e6) throw std::runtime_error("distr::Distribution: the tails are too heavy to sum over");
    if (!again) break;
  }
  if (disc) for (std::size_t i = 0; i < n; ++i) { lo[i] = std::floor(lo[i]); hi[i] = std::ceil(hi[i]); }
  for (int it = 0; it < (disc ? 80 : 90); ++it) {
    std::vector<double> mid(n);
    bool open = false;
    for (std::size_t i = 0; i < n; ++i) {
      mid[i] = disc ? std::floor(0.5 * (lo[i] + hi[i])) : 0.5 * (lo[i] + hi[i]);
      open = open || !bad[i] && (disc ? hi[i] - lo[i] > 1.0 : hi[i] - lo[i] > 1e-11 * std::max(1.0, std::abs(hi[i])));
    }
    if (!open) break;
    const auto c = cdf_at_(d, mid);
    for (std::size_t i = 0; i < n; ++i) {
      const bool done = disc ? hi[i] - lo[i] <= 1.0 : hi[i] - lo[i] <= 1e-11 * std::max(1.0, std::abs(hi[i]));
      if (done || bad[i]) continue;
      (c[i] >= ps[i] ? hi : lo)[i] = mid[i];
    }
  }
  std::vector<double> out(n);
  for (std::size_t i = 0; i < n; ++i)
    out[i] = bad[i] ? std::numeric_limits<double>::quiet_NaN() : (disc ? hi[i] : 0.5 * (lo[i] + hi[i]));
  return out;
}

struct Quad {
  double shallow_lo = -kInf, shallow_hi = kInf;  // the range between the 1e-10 and 1 - 1e-10 quantiles
  std::vector<double> x;
  std::vector<double> w;  // probability weights for a discrete family; width x GL weight for a continuous one
};

void gauss_legendre_(int n, std::vector<double>& nodes, std::vector<double>& weights) {
  nodes.assign(static_cast<std::size_t>(n), 0.0);
  weights.assign(static_cast<std::size_t>(n), 0.0);
  for (int i = 0; i < (n + 1) / 2; ++i) {
    double z = std::cos(std::numbers::pi * (i + 0.75) / (n + 0.5)), pp = 1.0;
    for (int it = 0; it < 100; ++it) {
      double p1 = 1.0, p2 = 0.0;
      for (int j = 0; j < n; ++j) {
        const double p3 = p2;
        p2 = p1;
        p1 = ((2.0 * j + 1.0) * z * p2 - j * p3) / (j + 1.0);
      }
      pp = n * (z * p1 - p2) / (z * z - 1.0);
      const double z1 = z;
      z = z1 - p1 / pp;
      if (std::abs(z - z1) < 1e-15) break;
    }
    nodes[static_cast<std::size_t>(i)] = -z;
    nodes[static_cast<std::size_t>(n - 1 - i)] = z;
    weights[static_cast<std::size_t>(i)] = 2.0 / ((1.0 - z * z) * pp * pp);
    weights[static_cast<std::size_t>(n - 1 - i)] = weights[static_cast<std::size_t>(i)];
  }
}

// Quadrature over the distribution: pieces of equal probability (finer in the tails), Gauss-Legendre on each; for a
// discrete family, the integers between the same quantile points.
Quad quadrature_(const Distribution& d, double tail) {
  std::vector<double> cuts;
  const int pieces = 24;
  std::vector<double> ps;
  for (double t = tail; t < 1e-3; t *= 10.0) ps.push_back(t);
  for (int i = 0; i <= pieces; ++i) ps.push_back(std::clamp(static_cast<double>(i) / pieces, 1e-3, 1.0 - 1e-3));
  std::vector<double> upper;
  for (double t = tail; t < 1e-3; t *= 10.0) upper.push_back(1.0 - t);
  std::sort(ps.begin(), ps.end());
  ps.erase(std::unique(ps.begin(), ps.end()), ps.end());
  ps.insert(ps.end(), upper.rbegin(), upper.rend());
  std::sort(ps.begin(), ps.end());
  ps.erase(std::unique(ps.begin(), ps.end()), ps.end());
  cuts = quantiles_(d, ps);
  {
    std::vector<double> ps2, cuts2;
    for (std::size_t i = 0; i < ps.size(); ++i) if (std::isfinite(cuts[i])) { ps2.push_back(ps[i]); cuts2.push_back(cuts[i]); }
    if (cuts2.size() < 4) throw std::runtime_error("distr::Distribution: the distribution function is too coarse to locate the tails");
    ps = ps2; cuts = cuts2;
  }
  std::sort(cuts.begin(), cuts.end());
  Quad q;
  {
    const auto find_cut = [&](double p) {
      std::size_t best = 0;
      for (std::size_t i = 0; i < ps.size(); ++i) if (std::abs(ps[i] - p) < std::abs(ps[best] - p)) best = i;
      return cuts[best];
    };
    q.shallow_lo = find_cut(1e-10);
    q.shallow_hi = find_cut(1.0 - 1e-10);
  }
  if (d.is_discrete()) {
    const double lo = std::floor(cuts.front()), hi = std::ceil(cuts.back());
    if (hi - lo > 4.0e6) throw std::runtime_error("distr::Distribution: the tails are too heavy to sum over");
    std::vector<double> xs;
    for (double k = lo; k <= hi; k += 1.0) xs.push_back(k);
    const Series pm = pdf(d.kind, f64_series_(xs), std::span<const double>(d.params));
    q.x = xs;
    q.w = to_doubles_(pm);
    return q;
  }
  std::vector<double> gx, gw;
  gauss_legendre_(16, gx, gw);
  for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
    const double a = cuts[i], b = cuts[i + 1];
    if (!(b > a)) continue;
    const double half = 0.5 * (b - a), mid = 0.5 * (a + b);
    for (std::size_t j = 0; j < gx.size(); ++j) {
      q.x.push_back(mid + half * gx[j]);
      q.w.push_back(half * gw[j]);
    }
  }
  const Series dens = pdf(d.kind, f64_series_(q.x), std::span<const double>(d.params));
  const auto dv = to_doubles_(dens);
  for (std::size_t i = 0; i < q.w.size(); ++i) q.w[i] *= dv[i];
  return q;
}

Quad shallow_of_(const Quad& q) {
  Quad out;
  for (std::size_t i = 0; i < q.x.size(); ++i)
    if (q.x[i] >= q.shallow_lo && q.x[i] <= q.shallow_hi) { out.x.push_back(q.x[i]); out.w.push_back(q.w[i]); }
  return out;
}

// Central moments mu_1 (the mean) .. mu_order of the quadrature, as raw mean and central moments about it.
std::vector<double> central_moments_(const Quad& q, int order) {
  double total = 0.0, mean = 0.0;
  for (std::size_t i = 0; i < q.x.size(); ++i) { total += q.w[i]; mean += q.w[i] * q.x[i]; }
  if (!(total > 0.0) || !std::isfinite(total)) throw std::runtime_error("distr::Distribution: the density integrates to a non-finite value");
  mean /= total;
  std::vector<double> mu(static_cast<std::size_t>(order) + 1, 0.0);
  mu[1] = mean;
  for (std::size_t i = 0; i < q.x.size(); ++i) {
    const double r = q.x[i] - mean;
    double pw = r;
    for (int k = 2; k <= order; ++k) { pw *= r; mu[static_cast<std::size_t>(k)] += q.w[i] * pw; }
  }
  for (int k = 2; k <= order; ++k) mu[static_cast<std::size_t>(k)] /= total;
  return mu;
}

double cumulant_from_central_(const std::vector<double>& mu, int order) {
  switch (order) {
    case 1: return mu[1];
    case 2: return mu[2];
    case 3: return mu[3];
    case 4: return mu[4] - 3.0 * mu[2] * mu[2];
    case 5: return mu[5] - 10.0 * mu[3] * mu[2];
    default: return mu[6] - 15.0 * mu[4] * mu[2] - 10.0 * mu[3] * mu[3] + 30.0 * mu[2] * mu[2] * mu[2];
  }
}

}  // namespace

bool Distribution::is_discrete() const noexcept {
  switch (kind) {
    case dst::Kind::Bernoulli: case dst::Kind::Binomial: case dst::Kind::Geometric:
    case dst::Kind::Hypergeometric: case dst::Kind::NegativeBinomial: case dst::Kind::Poisson:
    case dst::Kind::Rademacher: case dst::Kind::Skellam: case dst::Kind::Bessel: case dst::Kind::Yule:
    case dst::Kind::Zeta:
      return true;
    default:
      return false;
  }
}

Series Distribution::pdf(const Series& x) const { return dispatch_(*this, x, 0); }
Series Distribution::logpdf(const Series& x) const { return dispatch_(*this, x, 1); }
Series Distribution::cdf(const Series& x) const { return dispatch_(*this, x, 2); }
Series Distribution::logcdf(const Series& x) const { return dispatch_(*this, x, 3); }

Series Distribution::sample(std::span<const std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const {
  require_scalar_params_(*this, "sample");
  return distr::sample(kind, std::span<const double>(params), pluss::msharp::RangeView::Int{sizes.data(), sizes.size()}, cfg, seed);
}

Series Distribution::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const {
  return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed);
}

Series Distribution::sample_like(const Series& like, std::uint64_t seed) const {
  require_scalar_params_(*this, "sample_like");
  return distr::sample_like(kind, std::span<const double>(params), like, seed);
}

Series Distribution::cumulant(std::int64_t order) const {
  require_scalar_params_(*this, "cumulant");
  if (order < 1 || order > 6) throw std::invalid_argument("distr::Distribution::cumulant: the order must be between 1 and 6");
  const int k = static_cast<int>(order);
  const Quad full = quadrature_(*this, 1e-14);
  const double deep = cumulant_from_central_(central_moments_(full, k), k);
  const double shallow = cumulant_from_central_(central_moments_(shallow_of_(full), k), k);
  if (!std::isfinite(deep) || std::abs(deep - shallow) > 1e-3 * std::max(1.0, std::abs(deep)))
    throw std::runtime_error("distr::Distribution::cumulant: the moment of this order does not exist, or the tails are too heavy to integrate");
  return pluss::make_series(deep).to(ScalarType::Float64);
}

Series Distribution::cgf(const Series& t) const {
  require_scalar_params_(*this, "cgf");
  const Quad deep = quadrature_(*this, 1e-14);
  const Quad shallow = shallow_of_(deep);
  const auto tv = to_doubles_(t.reshape({t.numel()}));
  std::vector<double> out;
  out.reserve(tv.size());
  const auto lse = [](const Quad& q, double tt) {
    double mx = -kInf;
    std::vector<double> terms(q.x.size());
    for (std::size_t i = 0; i < q.x.size(); ++i) {
      terms[i] = q.w[i] > 0.0 ? std::log(q.w[i]) + tt * q.x[i] : -kInf;
      mx = std::max(mx, terms[i]);
    }
    double s = 0.0;
    for (double v : terms) s += std::exp(v - mx);
    return mx + std::log(s);
  };
  for (double tt : tv) {
    const double a = lse(deep, tt), b = lse(shallow, tt);
    if (!std::isfinite(a) || std::abs(a - b) > 1e-3 * std::max(1.0, std::abs(a)))
      throw std::runtime_error("distr::Distribution::cgf: the moment generating function does not exist at this t, or the tails are too heavy to integrate");
    out.push_back(a - lse(deep, 0.0));
  }
  Series r = f64_series_(out);
  return t.dim() == 0 ? r.reshape(std::vector<std::int64_t>{}) : r.reshape(std::vector<std::int64_t>(t.sizes().begin(), t.sizes().end()));
}

namespace {

double neg_loglik_(const Distribution& base, const Series& x, const std::vector<double>& p) {
  try {
    Distribution d = base;
    d.params = p;
    d.series_params.clear();
    const auto lv = to_doubles_(d.logpdf(x));
    double s = 0.0;
    for (double v : lv) s += v;
    return std::isfinite(s) ? -s : kInf;
  } catch (...) {
    return kInf;
  }
}

std::vector<double> nelder_mead_(const std::function<double(const std::vector<double>&)>& f, std::vector<double> x0,
                                 std::int64_t& iterations, bool& converged) {
  const std::size_t n = x0.size();
  std::vector<std::vector<double>> sim(n + 1, x0);
  for (std::size_t i = 0; i < n; ++i) sim[i + 1][i] += (std::abs(x0[i]) > 1e-8 ? 0.1 * std::abs(x0[i]) : 0.05);
  std::vector<double> fv(n + 1);
  for (std::size_t i = 0; i <= n; ++i) fv[i] = f(sim[i]);
  iterations = 0;
  converged = false;
  for (int it = 0; it < 4000 * static_cast<int>(n + 1); ++it) {
    iterations = it;
    std::vector<std::size_t> idx(n + 1);
    for (std::size_t i = 0; i <= n; ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) { return fv[a] < fv[b]; });
    std::vector<std::vector<double>> s2; std::vector<double> f2;
    for (auto i : idx) { s2.push_back(sim[i]); f2.push_back(fv[i]); }
    sim = s2; fv = f2;
    double spread = 0.0;
    for (std::size_t i = 1; i <= n; ++i)
      for (std::size_t j = 0; j < n; ++j) spread = std::max(spread, std::abs(sim[i][j] - sim[0][j]) / std::max(1.0, std::abs(sim[0][j])));
    if (spread < 1e-10 && std::isfinite(fv[0]) && std::abs(fv[n] - fv[0]) < 1e-12 * std::max(1.0, std::abs(fv[0]))) { converged = true; break; }
    std::vector<double> c(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) for (std::size_t j = 0; j < n; ++j) c[j] += sim[i][j] / static_cast<double>(n);
    const auto pt = [&](double a) { std::vector<double> r(n); for (std::size_t j = 0; j < n; ++j) r[j] = c[j] + a * (sim[n][j] - c[j]); return r; };
    auto xr = pt(-1.0); const double fr = f(xr);
    if (fr < fv[0]) {
      auto xe = pt(-2.0); const double fe = f(xe);
      if (fe < fr) { sim[n] = xe; fv[n] = fe; } else { sim[n] = xr; fv[n] = fr; }
    } else if (fr < fv[n - 1]) { sim[n] = xr; fv[n] = fr; }
    else {
      auto xc = pt(fr < fv[n] ? -0.5 : 0.5); const double fc = f(xc);
      if (fc < std::min(fr, fv[n])) { sim[n] = xc; fv[n] = fc; }
      else for (std::size_t i = 1; i <= n; ++i) { for (std::size_t j = 0; j < n; ++j) sim[i][j] = sim[0][j] + 0.5 * (sim[i][j] - sim[0][j]); fv[i] = f(sim[i]); }
    }
  }
  std::size_t best = 0;
  for (std::size_t i = 1; i <= n; ++i) if (fv[i] < fv[best]) best = i;
  return sim[best];
}

bool invert_(std::vector<double> a, std::size_t n, std::vector<double>& inv) {
  inv.assign(n * n, 0.0);
  for (std::size_t i = 0; i < n; ++i) inv[i * n + i] = 1.0;
  for (std::size_t c = 0; c < n; ++c) {
    std::size_t piv = c;
    for (std::size_t r = c + 1; r < n; ++r) if (std::abs(a[r * n + c]) > std::abs(a[piv * n + c])) piv = r;
    if (std::abs(a[piv * n + c]) < 1e-300) return false;
    if (piv != c) for (std::size_t j = 0; j < n; ++j) { std::swap(a[c * n + j], a[piv * n + j]); std::swap(inv[c * n + j], inv[piv * n + j]); }
    const double d = a[c * n + c];
    for (std::size_t j = 0; j < n; ++j) { a[c * n + j] /= d; inv[c * n + j] /= d; }
    for (std::size_t r = 0; r < n; ++r) if (r != c) {
      const double m = a[r * n + c];
      for (std::size_t j = 0; j < n; ++j) { a[r * n + j] -= m * a[c * n + j]; inv[r * n + j] -= m * inv[c * n + j]; }
    }
  }
  return true;
}

double normal_quantile_(double p) {
  double lo = -40.0, hi = 40.0;
  for (int i = 0; i < 200; ++i) {
    const double mid = 0.5 * (lo + hi);
    (0.5 * std::erfc(-mid / std::numbers::sqrt2) < p ? lo : hi) = mid;
  }
  return 0.5 * (lo + hi);
}

}  // namespace


namespace detail {

double generic_loglik_(dst::Kind kind, const std::vector<double>& params, const Series& x) {
  return -neg_loglik_(Distribution(kind, params), x, params);
}

std::vector<double> generic_vcov_(dst::Kind kind, const std::vector<double>& params, const Series& x) {
  const Distribution base(kind, params);
  const Series xs = x.to(pluss::pCPU, ScalarType::Float64).contiguous().reshape({x.numel()});
  const auto f = [&](const std::vector<double>& p) { return neg_loglik_(base, xs, p); };
  const std::vector<double> best = params;
  const std::size_t n = best.size();
  std::vector<double> H(n * n, 0.0);
  std::vector<double> h(n);
  for (std::size_t i = 0; i < n; ++i) h[i] = 1e-4 * std::max(std::abs(best[i]), 1e-2);
  const double f0 = f(best);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = i; j < n; ++j) {
      auto at = [&](double di, double dj) { auto p = best; p[i] += di; p[j] += dj; return f(p); };
      double v;
      if (i == j) v = (at(h[i], 0) - 2.0 * f0 + at(-h[i], 0)) / (h[i] * h[i]);
      else v = (at(h[i], h[j]) - at(h[i], -h[j]) - at(-h[i], h[j]) + at(-h[i], -h[j])) / (4.0 * h[i] * h[j]);
      H[i * n + j] = H[j * n + i] = v;
    }
  }
  std::vector<double> inv;
  bool ok = std::isfinite(f0);
  for (double v : H) ok = ok && std::isfinite(v);
  if (ok && invert_(H, n, inv)) {
    bool pd = true;
    for (std::size_t i = 0; i < n; ++i) pd = pd && inv[i * n + i] > 0.0 && std::isfinite(inv[i * n + i]);
    if (pd) return inv;
  }
  return {};
}

}  // namespace detail

DistributionFit Distribution::MLE(const Series& x, std::vector<double> start) const {
  require_scalar_params_(*this, "MLE");
  if (start.empty()) start = params;
  if (start.empty()) throw std::invalid_argument("distr::Distribution::MLE: a starting parameter list is required");
  const Series xs = x.to(pluss::pCPU, ScalarType::Float64).contiguous().reshape({x.numel()});
  const auto f = [&](const std::vector<double>& p) { return neg_loglik_(*this, xs, p); };
  if (!std::isfinite(f(start))) throw std::invalid_argument("distr::Distribution::MLE: the starting parameters give a non-finite likelihood");
  DistributionFit fit;
  fit.kind = kind;
  std::int64_t iters = 0; bool conv = false;
  auto best = nelder_mead_(f, start, iters, conv);
  for (int restart = 0; restart < 3; ++restart) {  // restart from the best point to escape a collapsed simplex
    std::int64_t it2 = 0; bool c2 = false;
    auto again = nelder_mead_(f, best, it2, c2);
    iters += it2; conv = c2;
    if (f(again) < f(best)) best = again; else break;
  }
  fit.params = best;
  fit.loglik = -f(best);
  fit.converged = conv;
  fit.iterations = iters;
  fit.vcov = detail::generic_vcov_(kind, best, xs);
  return fit;
}

Distribution DistributionFit::model() const { return Distribution(kind, params); }

std::vector<double> DistributionFit::stderr() const {
  std::vector<double> se;
  const std::size_t n = params.size();
  for (std::size_t i = 0; i < n; ++i)
    se.push_back(vcov.empty() ? std::numeric_limits<double>::quiet_NaN() : std::sqrt(vcov[i * n + i]));
  return se;
}

std::vector<std::pair<double, double>> DistributionFit::confint(double level) const {
  if (!(level > 0.0 && level < 1.0)) throw std::invalid_argument("distr::DistributionFit::confint: level must be in (0, 1)");
  const double z = normal_quantile_(0.5 + 0.5 * level);
  const auto se = stderr();
  std::vector<std::pair<double, double>> out;
  for (std::size_t i = 0; i < params.size(); ++i) out.emplace_back(params[i] - z * se[i], params[i] + z * se[i]);
  return out;
}

}  // namespace pluss::subsp::stats::distr
