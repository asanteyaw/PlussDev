// Dedicated objects for the heavy-tailed families Pareto, Lomax, Burr, Zeta and Yule. Their densities,
// distribution functions and draws come from the kind-based functions through distr::Distribution; what these
// objects add is the exact cumulants, which exist only below the tail index, the cumulant generating function
// where it exists (t <= 0) and a maximum-likelihood fit.
#include "pluss/subsp/stats/distr.h"

#include "distr_generic_detail.h"
#include "distr_object_detail.h"

#include "pluss/tils/auxiliary/scalar_cast.h"

#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <type_traits>

namespace pluss::subsp::stats::distr {

namespace {

using pluss::ScalarType;
using pluss::Series;

double lgamma_(double x) { return std::lgamma(x); }

double zeta_(double s) {
  const Series v = pluss::make_series(std::vector<double>{s}, {1}).to(ScalarType::Float64).zeta(1.0);
  return v.contiguous().host<double>().front();
}

[[noreturn]] void no_moment_(const char* who, const char* condition, std::int64_t order) {
  throw std::invalid_argument(std::string(who) + "::cumulant: the moment of order " + std::to_string(order) +
                              " does not exist; it needs " + condition);
}

// ---- the five families: kind, parameters, exact raw moments --------------------------------------------------

struct ParetoInfo {
  static constexpr dst::Kind kind = dst::Kind::Pareto;
  static constexpr bool discrete = false;
  static constexpr const char* name = "Pareto";
  static double raw(const std::vector<double>& p, std::int64_t n) {
    if (!(p[1] > static_cast<double>(n))) no_moment_(name, "alpha > the order", n);
    return p[1] * std::pow(p[0], static_cast<double>(n)) / (p[1] - static_cast<double>(n));
  }
};
struct LomaxInfo {
  static constexpr dst::Kind kind = dst::Kind::Lomax;
  static constexpr bool discrete = false;
  static constexpr const char* name = "Lomax";
  static double raw(const std::vector<double>& p, std::int64_t n) {
    if (!(p[1] > static_cast<double>(n))) no_moment_(name, "alpha > the order", n);
    double m = std::pow(p[0], static_cast<double>(n));
    for (std::int64_t j = 1; j <= n; ++j) m *= static_cast<double>(j) / (p[1] - static_cast<double>(j));
    return m;
  }
};
struct BurrInfo {
  static constexpr dst::Kind kind = dst::Kind::Burr;
  static constexpr bool discrete = false;
  static constexpr const char* name = "Burr";
  static double raw(const std::vector<double>& p, std::int64_t n) {
    const double c = p[0], k = p[1], r = static_cast<double>(n) / c;
    if (!(r < 1.0)) no_moment_(name, "c > the order", n);
    return std::exp(std::log(k) + lgamma_(k + r) + lgamma_(1.0 - r) - lgamma_(k + 1.0));
  }
};
struct ZetaInfo {
  static constexpr dst::Kind kind = dst::Kind::Zeta;
  static constexpr bool discrete = true;
  static constexpr const char* name = "Zeta";
  static double raw(const std::vector<double>& p, std::int64_t n) {
    if (!(p[0] > static_cast<double>(n) + 1.0)) no_moment_(name, "s > the order + 1", n);
    return zeta_(p[0] - static_cast<double>(n)) / zeta_(p[0]);
  }
};
struct YuleInfo {
  static constexpr dst::Kind kind = dst::Kind::Yule;
  static constexpr bool discrete = true;
  static constexpr const char* name = "Yule";
  // With rho = p - 1 the rising factorial moment E[K (K+1) ... (K+j-1)] is rho j! / (rho - j) for rho > j; the raw
  // moments follow from the Stirling numbers of the second kind.
  static double raw(const std::vector<double>& p, std::int64_t n) {
    const double rho = p[0] - 1.0;
    if (!(rho > static_cast<double>(n))) no_moment_(name, "p > the order + 1", n);
    const auto rising = [&](std::int64_t j) { return rho * detail::factorial(j) / (rho - static_cast<double>(j)); };
    std::vector<std::vector<double>> S(static_cast<std::size_t>(n) + 1, std::vector<double>(static_cast<std::size_t>(n) + 1, 0.0));
    S[0][0] = 1.0;
    for (std::int64_t i = 1; i <= n; ++i)
      for (std::int64_t j = 1; j <= i; ++j)
        S[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)] =
            static_cast<double>(j) * S[static_cast<std::size_t>(i - 1)][static_cast<std::size_t>(j)] + S[static_cast<std::size_t>(i - 1)][static_cast<std::size_t>(j - 1)];
    double m = 0.0;
    for (std::int64_t j = 1; j <= n; ++j)
      m += ((n - j) % 2 == 0 ? 1.0 : -1.0) * S[static_cast<std::size_t>(n)][static_cast<std::size_t>(j)] * rising(j);
    return m;
  }
};

std::vector<double> params_of_(const Pareto& o) { return {o.xm, o.alpha}; }
std::vector<double> params_of_(const Lomax& o) { return {o.scale, o.alpha}; }
std::vector<double> params_of_(const Burr& o) { return {o.c, o.k}; }
std::vector<double> params_of_(const Zeta& o) { return {o.s}; }
std::vector<double> params_of_(const Yule& o) { return {o.p}; }


template <class Info, class Obj>
Distribution dist_of_(const Obj& o) {
  return Distribution(Info::kind, params_of_(o));
}

template <class Info, class Obj>
Series cumulant_(const Obj& o, std::int64_t order) {
  detail::require_positive_order(order, std::string(Info::name) + "::cumulant");
  const auto p = params_of_(o);
  std::vector<double> raw(static_cast<std::size_t>(order) + 1, 0.0);
  raw[0] = 1.0;
  for (std::int64_t n = 1; n <= order; ++n) raw[static_cast<std::size_t>(n)] = Info::raw(p, n);
  return detail::scalar_f64(detail::cumulants_from_raw_moments(raw)[static_cast<std::size_t>(order)]);
}

// log E[exp(t X)] exists for t <= 0 (a heavy right tail makes it diverge for t > 0).
template <class Info, class Obj>
Series cgf_(const Obj& o, const Series& t) {
  const auto tv = t.to(pluss::pCPU, ScalarType::Float64).contiguous().reshape({t.numel()}).host<double>().array().data;
  for (double v : tv)
    if (v > 0.0) throw std::invalid_argument(std::string(Info::name) + "::cgf: the moment generating function does not exist for t > 0");
  const auto p = params_of_(o);
  std::vector<double> out;
  out.reserve(tv.size());
  for (double tt : tv) {
    if (tt == 0.0) { out.push_back(0.0); continue; }
    if constexpr (Info::discrete) {
      // Sum the masses weighted by exp(t k) until the weight is negligible.
      const double kmax = std::min(4.0e6, std::ceil(745.0 / -tt) + 1.0);
      std::vector<double> ks;
      for (double k = 1.0; k <= kmax; k += 1.0) ks.push_back(k);
      const Series pm = pdf(Info::kind, pluss::make_series(ks, {static_cast<std::int64_t>(ks.size())}).to(ScalarType::Float64), std::span<const double>(p));
      const auto pv = pm.contiguous().host<double>().array().data;
      double s = 0.0;
      for (std::size_t i = 0; i < ks.size(); ++i) s += pv[i] * std::exp(tt * ks[i]);
      out.push_back(std::log(s));
    } else {
      out.push_back(Distribution(Info::kind, p).cgf(pluss::make_series(tt).to(ScalarType::Float64)).contiguous().host<double>().front());
    }
  }
  Series r = pluss::make_series(out, {static_cast<std::int64_t>(out.size())}).to(ScalarType::Float64);
  return t.dim() == 0 ? r.reshape(std::vector<std::int64_t>{}) : r.reshape(std::vector<std::int64_t>(t.sizes().begin(), t.sizes().end()));
}

template <class Fit>
Series loglik_(dst::Kind kind, const std::vector<double>& p, const Series& x) {
  return detail::scalar_f64(detail::generic_loglik_(kind, p, x));
}

std::vector<double> vcov_checked_(dst::Kind kind, const std::vector<double>& p, const Series& x, const char* who) {
  auto v = detail::generic_vcov_(kind, p, x);
  if (v.empty()) throw std::runtime_error(std::string(who) + ": the observed information is not positive definite at the estimate");
  return v;
}

Series vcov_series_(const std::vector<double>& v, std::size_t n) {
  return pluss::make_series(v, {static_cast<std::int64_t>(n), static_cast<std::int64_t>(n)}).to(ScalarType::Float64);
}

Series stderr_series_(const std::vector<double>& v, std::size_t n) {
  std::vector<double> se;
  for (std::size_t i = 0; i < n; ++i) se.push_back(std::sqrt(v[i * n + i]));
  return pluss::make_series(se, {static_cast<std::int64_t>(n)}).to(ScalarType::Float64);
}

double normal_quantile_(double p) {
  double lo = -40.0, hi = 40.0;
  for (int i = 0; i < 200; ++i) {
    const double mid = 0.5 * (lo + hi);
    (0.5 * std::erfc(-mid / std::numbers::sqrt2) < p ? lo : hi) = mid;
  }
  return 0.5 * (lo + hi);
}

Series confint_series_(const std::vector<double>& est, const std::vector<double>& v, double level, const char* who) {
  if (!(level > 0.0 && level < 1.0)) throw std::invalid_argument(std::string(who) + ": level must be in (0, 1)");
  const double z = normal_quantile_(0.5 + 0.5 * level);
  const std::size_t n = est.size();
  std::vector<double> out;
  for (std::size_t i = 0; i < n; ++i) {
    const double se = std::sqrt(v[i * n + i]);
    out.push_back(est[i] - z * se);
    out.push_back(est[i] + z * se);
  }
  return pluss::make_series(out, {static_cast<std::int64_t>(n), 2}).to(ScalarType::Float64);
}

template <class Info, class Fit>
Fit numeric_fit_(const Distribution& start_dist, const Series& x, std::vector<double> start) {
  const auto r = start_dist.MLE(x, std::move(start));
  if constexpr (std::is_same_v<Fit, ZetaFit> || std::is_same_v<Fit, YuleFit>)
    return Fit{r.params[0], r.converged, r.iterations};
  else
    return Fit{r.params[0], r.params[1], r.converged, r.iterations};
}

}  // namespace

// ---- Pareto --------------------------------------------------------------------------------------------------
Series Pareto::sample(std::span<const std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return dist_of_<ParetoInfo>(*this).sample(sizes, cfg, seed); }
Series Pareto::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed); }
Series Pareto::sample_like(const Series& like, std::uint64_t seed) const { return dist_of_<ParetoInfo>(*this).sample_like(like, seed); }
Series Pareto::pdf(const Series& x) const { return dist_of_<ParetoInfo>(*this).pdf(x); }
Series Pareto::logpdf(const Series& x) const { return dist_of_<ParetoInfo>(*this).logpdf(x); }
Series Pareto::logcdf(const Series& x) const { return dist_of_<ParetoInfo>(*this).logcdf(x); }
Series Pareto::cdf(const Series& x) const { return dist_of_<ParetoInfo>(*this).cdf(x); }
Series Pareto::cgf(const Series& t) const { return cgf_<ParetoInfo>(*this, t); }
Series Pareto::cumulant(std::int64_t order) const { return cumulant_<ParetoInfo>(*this, order); }
ParetoFit Pareto::MLE(const Series& x) const {
  const auto v = x.to(pluss::pCPU, ScalarType::Float64).contiguous().reshape({x.numel()}).host<double>().array().data;
  if (v.empty()) throw std::invalid_argument("Pareto::MLE: no observations");
  double xm = v.front();
  for (double e : v) xm = std::min(xm, e);
  if (!(xm > 0.0)) throw std::invalid_argument("Pareto::MLE: the observations must be positive");
  double s = 0.0;
  for (double e : v) s += std::log(e / xm);
  if (!(s > 0.0)) throw std::runtime_error("Pareto::MLE: the observations are all equal");
  return ParetoFit{xm, static_cast<double>(v.size()) / s, true, 0};
}
Pareto ParetoFit::model() const { return Pareto{xm, alpha}; }
Series ParetoFit::loglik(const Series& x) const { return loglik_<ParetoFit>(dst::Kind::Pareto, {xm, alpha}, x); }
// The minimum is a boundary estimate, so the usual observed information does not exist: n (xm_hat - xm) / xm is
// asymptotically exponential with rate alpha, which gives the variance (xm / (n alpha))^2, and the tail index has
// variance alpha^2 / n. The two are asymptotically independent.
std::vector<double> pareto_vcov_(const ParetoFit& f, const Series& x) {
  const double n = static_cast<double>(x.numel());
  const double sx = f.xm / (n * f.alpha);
  return {sx * sx, 0.0, 0.0, f.alpha * f.alpha / n};
}
Series ParetoFit::vcov(const Series& x) const { return vcov_series_(pareto_vcov_(*this, x), 2); }
Series ParetoFit::stderr(const Series& x) const { return stderr_series_(pareto_vcov_(*this, x), 2); }
Series ParetoFit::confint(const Series& x, double level) const { return confint_series_({xm, alpha}, pareto_vcov_(*this, x), level, "ParetoFit::confint"); }

// ---- Lomax ---------------------------------------------------------------------------------------------------
Series Lomax::sample(std::span<const std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return dist_of_<LomaxInfo>(*this).sample(sizes, cfg, seed); }
Series Lomax::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed); }
Series Lomax::sample_like(const Series& like, std::uint64_t seed) const { return dist_of_<LomaxInfo>(*this).sample_like(like, seed); }
Series Lomax::pdf(const Series& x) const { return dist_of_<LomaxInfo>(*this).pdf(x); }
Series Lomax::logpdf(const Series& x) const { return dist_of_<LomaxInfo>(*this).logpdf(x); }
Series Lomax::logcdf(const Series& x) const { return dist_of_<LomaxInfo>(*this).logcdf(x); }
Series Lomax::cdf(const Series& x) const { return dist_of_<LomaxInfo>(*this).cdf(x); }
Series Lomax::cgf(const Series& t) const { return cgf_<LomaxInfo>(*this, t); }
Series Lomax::cumulant(std::int64_t order) const { return cumulant_<LomaxInfo>(*this, order); }
LomaxFit Lomax::MLE(const Series& x, std::vector<double> start) const {
  if (start.empty()) start = params_of_(*this);
  return numeric_fit_<LomaxInfo, LomaxFit>(Distribution(dst::Kind::Lomax, params_of_(*this)), x, std::move(start));
}
Lomax LomaxFit::model() const { return Lomax{scale, alpha}; }
Series LomaxFit::loglik(const Series& x) const { return loglik_<LomaxFit>(dst::Kind::Lomax, {scale, alpha}, x); }
Series LomaxFit::vcov(const Series& x) const { return vcov_series_(vcov_checked_(dst::Kind::Lomax, {scale, alpha}, x, "LomaxFit::vcov"), 2); }
Series LomaxFit::stderr(const Series& x) const { return stderr_series_(vcov_checked_(dst::Kind::Lomax, {scale, alpha}, x, "LomaxFit::stderr"), 2); }
Series LomaxFit::confint(const Series& x, double level) const { return confint_series_({scale, alpha}, vcov_checked_(dst::Kind::Lomax, {scale, alpha}, x, "LomaxFit::confint"), level, "LomaxFit::confint"); }

// ---- Burr ----------------------------------------------------------------------------------------------------
Series Burr::sample(std::span<const std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return dist_of_<BurrInfo>(*this).sample(sizes, cfg, seed); }
Series Burr::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed); }
Series Burr::sample_like(const Series& like, std::uint64_t seed) const { return dist_of_<BurrInfo>(*this).sample_like(like, seed); }
Series Burr::pdf(const Series& x) const { return dist_of_<BurrInfo>(*this).pdf(x); }
Series Burr::logpdf(const Series& x) const { return dist_of_<BurrInfo>(*this).logpdf(x); }
Series Burr::logcdf(const Series& x) const { return dist_of_<BurrInfo>(*this).logcdf(x); }
Series Burr::cdf(const Series& x) const { return dist_of_<BurrInfo>(*this).cdf(x); }
Series Burr::cgf(const Series& t) const { return cgf_<BurrInfo>(*this, t); }
Series Burr::cumulant(std::int64_t order) const { return cumulant_<BurrInfo>(*this, order); }
BurrFit Burr::MLE(const Series& x, std::vector<double> start) const {
  if (start.empty()) start = params_of_(*this);
  return numeric_fit_<BurrInfo, BurrFit>(Distribution(dst::Kind::Burr, params_of_(*this)), x, std::move(start));
}
Burr BurrFit::model() const { return Burr{c, k}; }
Series BurrFit::loglik(const Series& x) const { return loglik_<BurrFit>(dst::Kind::Burr, {c, k}, x); }
Series BurrFit::vcov(const Series& x) const { return vcov_series_(vcov_checked_(dst::Kind::Burr, {c, k}, x, "BurrFit::vcov"), 2); }
Series BurrFit::stderr(const Series& x) const { return stderr_series_(vcov_checked_(dst::Kind::Burr, {c, k}, x, "BurrFit::stderr"), 2); }
Series BurrFit::confint(const Series& x, double level) const { return confint_series_({c, k}, vcov_checked_(dst::Kind::Burr, {c, k}, x, "BurrFit::confint"), level, "BurrFit::confint"); }

// ---- Zeta ----------------------------------------------------------------------------------------------------
Series Zeta::sample(std::span<const std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return dist_of_<ZetaInfo>(*this).sample(sizes, cfg, seed); }
Series Zeta::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed); }
Series Zeta::sample_like(const Series& like, std::uint64_t seed) const { return dist_of_<ZetaInfo>(*this).sample_like(like, seed); }
Series Zeta::pdf(const Series& x) const { return dist_of_<ZetaInfo>(*this).pdf(x); }
Series Zeta::logpdf(const Series& x) const { return dist_of_<ZetaInfo>(*this).logpdf(x); }
Series Zeta::logcdf(const Series& x) const { return dist_of_<ZetaInfo>(*this).logcdf(x); }
Series Zeta::cdf(const Series& x) const { return dist_of_<ZetaInfo>(*this).cdf(x); }
Series Zeta::cgf(const Series& t) const { return cgf_<ZetaInfo>(*this, t); }
Series Zeta::cumulant(std::int64_t order) const { return cumulant_<ZetaInfo>(*this, order); }
ZetaFit Zeta::MLE(const Series& x, std::vector<double> start) const {
  if (start.empty()) start = params_of_(*this);
  return numeric_fit_<ZetaInfo, ZetaFit>(Distribution(dst::Kind::Zeta, params_of_(*this)), x, std::move(start));
}
Zeta ZetaFit::model() const { return Zeta{s}; }
Series ZetaFit::loglik(const Series& x) const { return loglik_<ZetaFit>(dst::Kind::Zeta, {s}, x); }
Series ZetaFit::vcov(const Series& x) const { return vcov_series_(vcov_checked_(dst::Kind::Zeta, {s}, x, "ZetaFit::vcov"), 1); }
Series ZetaFit::stderr(const Series& x) const { return stderr_series_(vcov_checked_(dst::Kind::Zeta, {s}, x, "ZetaFit::stderr"), 1); }
Series ZetaFit::confint(const Series& x, double level) const { return confint_series_({s}, vcov_checked_(dst::Kind::Zeta, {s}, x, "ZetaFit::confint"), level, "ZetaFit::confint"); }

// ---- Yule ----------------------------------------------------------------------------------------------------
Series Yule::sample(std::span<const std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return dist_of_<YuleInfo>(*this).sample(sizes, cfg, seed); }
Series Yule::sample(std::initializer_list<std::int64_t> sizes, pluss::Config cfg, std::uint64_t seed) const { return sample(std::span<const std::int64_t>(sizes.begin(), sizes.size()), cfg, seed); }
Series Yule::sample_like(const Series& like, std::uint64_t seed) const { return dist_of_<YuleInfo>(*this).sample_like(like, seed); }
Series Yule::pdf(const Series& x) const { return dist_of_<YuleInfo>(*this).pdf(x); }
Series Yule::logpdf(const Series& x) const { return dist_of_<YuleInfo>(*this).logpdf(x); }
Series Yule::logcdf(const Series& x) const { return dist_of_<YuleInfo>(*this).logcdf(x); }
Series Yule::cdf(const Series& x) const { return dist_of_<YuleInfo>(*this).cdf(x); }
Series Yule::cgf(const Series& t) const { return cgf_<YuleInfo>(*this, t); }
Series Yule::cumulant(std::int64_t order) const { return cumulant_<YuleInfo>(*this, order); }
YuleFit Yule::MLE(const Series& x, std::vector<double> start) const {
  if (start.empty()) start = params_of_(*this);
  return numeric_fit_<YuleInfo, YuleFit>(Distribution(dst::Kind::Yule, params_of_(*this)), x, std::move(start));
}
Yule YuleFit::model() const { return Yule{p}; }
Series YuleFit::loglik(const Series& x) const { return loglik_<YuleFit>(dst::Kind::Yule, {p}, x); }
Series YuleFit::vcov(const Series& x) const { return vcov_series_(vcov_checked_(dst::Kind::Yule, {p}, x, "YuleFit::vcov"), 1); }
Series YuleFit::stderr(const Series& x) const { return stderr_series_(vcov_checked_(dst::Kind::Yule, {p}, x, "YuleFit::stderr"), 1); }
Series YuleFit::confint(const Series& x, double level) const { return confint_series_({p}, vcov_checked_(dst::Kind::Yule, {p}, x, "YuleFit::confint"), level, "YuleFit::confint"); }

}  // namespace pluss::subsp::stats::distr
