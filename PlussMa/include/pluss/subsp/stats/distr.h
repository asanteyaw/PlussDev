#pragma once

#include "pluss/subsp/stats/distr_fwd.h"

namespace pluss::subsp::stats::distr {

namespace detail {
inline pluss::msharp::RangeView::Double confint_series_to_view_(const pluss::Series& ci) {
  thread_local pluss::Series cache = pluss::series::undefined();
  cache = ci.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().reshape({ci.numel()});
  const auto* ptr = reinterpret_cast<const double*>(cache.raw_data());
  return pluss::msharp::RangeView::Double{
      ptr, static_cast<std::size_t>(cache.numel())};
}
} // namespace detail

#define PLUSS_DISTR_CONFINT_VIEW_METHOD()                                                            \
  pluss::msharp::RangeView::Double confint_view(const pluss::Series& x, double level = 0.95) const { \
    return detail::confint_series_to_view_(confint(x, level));                                       \
  }

struct Normal;
struct Bernoulli;
struct Binomial;
struct Geometric;
struct Lognormal;
struct Uniform;
struct Exponential;
struct Rayleigh;
struct Poisson;
struct Laplace;
struct InverseGaussian;
struct Gamma;
struct Beta;
struct NegativeBinomial;
struct Nig;
struct Gh;
struct GaussianMixture;

struct CODEM_API NormalFit {
  double mean = 0.0;
  double stddev = 1.0;

  Normal model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API ExponentialFit {
  double rate = 1.0;

  Exponential model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API PoissonFit {
  double lambda = 1.0;

  Poisson model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API BernoulliFit {
  double p = 0.5;

  Bernoulli model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API BinomialFit {
  std::int64_t count = 1;
  double p = 0.5;

  Binomial model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API GeometricFit {
  double p = 0.5;

  Geometric model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API LognormalFit {
  double mean_log = 0.0;
  double stddev_log = 1.0;

  Lognormal model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API UniformFit {
  double low = 0.0;
  double high = 1.0;

  Uniform model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API RayleighFit {
  double sigma = 1.0;

  Rayleigh model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API LaplaceFit {
  double location = 0.0;
  double scale = 1.0;

  Laplace model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API InverseGaussianFit {
  double theta = 1.0;
  double chi = 1.0;

  InverseGaussian model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API GammaFit {
  double shape = 1.0;
  double scale = 1.0;
  double location = 0.0;

  Gamma model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API BetaFit {
  double low = 0.0;
  double high = 1.0;
  double alpha = 1.0;
  double beta = 1.0;

  Beta model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API NegativeBinomialFit {
  double r = 1.0;
  double p = 0.5;

  NegativeBinomial model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API NigFit {
  double alpha = 1.0;
  double beta = 0.0;
  double location = 0.0;
  double scale = 1.0;

  Nig model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API GhFit {
  double lambda = 1.0;
  double alpha = 2.0;
  double beta = 0.0;
  double location = 0.0;
  double scale = 1.0;

  Gh model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

struct CODEM_API GaussianMixtureFit {
  pluss::Series weights = pluss::series::undefined();
  pluss::Series means = pluss::series::undefined();
  pluss::Series stddevs = pluss::series::undefined();

  GaussianMixture model() const;
  pluss::Series loglik(const pluss::Series& x) const;
  pluss::Series loglik_per_obs(const pluss::Series& x) const;
  pluss::Series nll(const pluss::Series& x) const;
  pluss::Series score(const pluss::Series& x) const;
  pluss::Series fisher_inf(const pluss::Series& x) const;
  pluss::Series observed_inf(const pluss::Series& x) const;
  pluss::Series vcov(const pluss::Series& x) const;
  pluss::Series stderr(const pluss::Series& x) const;
  pluss::Series confint(const pluss::Series& x, double level = 0.95) const;
  PLUSS_DISTR_CONFINT_VIEW_METHOD()
};

#undef PLUSS_DISTR_CONFINT_VIEW_METHOD

struct CODEM_API Normal {
  double mean = 0.0;
  double stddev = 1.0;
  pluss::Series mean_series = pluss::series::undefined();
  pluss::Series stddev_series = pluss::series::undefined();

  Normal() = default;
  Normal(double mean_, double stddev_) : mean(mean_), stddev(stddev_) {}
  Normal(const pluss::Series& mean_series_, const pluss::Series& stddev_series_)
      : mean_series(mean_series_), stddev_series(stddev_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  NormalFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Bernoulli {
  double p = 0.5;
  pluss::Series p_series = pluss::series::undefined();

  Bernoulli() = default;
  explicit Bernoulli(double p_) : p(p_) {}
  explicit Bernoulli(const pluss::Series& p_series_) : p_series(p_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  BernoulliFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Binomial {
  std::int64_t count = 1;
  double p = 0.5;
  pluss::Series count_series = pluss::series::undefined();
  pluss::Series p_series = pluss::series::undefined();

  Binomial() = default;
  Binomial(std::int64_t count_, double p_) : count(count_), p(p_) {}
  Binomial(const pluss::Series& count_series_, const pluss::Series& p_series_)
      : count_series(count_series_), p_series(p_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  BinomialFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Poisson {
  double lambda = 1.0;
  pluss::Series lambda_series = pluss::series::undefined();

  Poisson() = default;
  explicit Poisson(double lambda_) : lambda(lambda_) {}
  explicit Poisson(const pluss::Series& lambda_series_) : lambda_series(lambda_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  PoissonFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Geometric {
  double p = 0.5;
  pluss::Series p_series = pluss::series::undefined();

  Geometric() = default;
  explicit Geometric(double p_) : p(p_) {}
  explicit Geometric(const pluss::Series& p_series_) : p_series(p_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  GeometricFit MLE(const pluss::Series& x) const;
};

struct CODEM_API NegativeBinomial {
  double r = 1.0;
  double p = 0.5;
  pluss::Series r_series = pluss::series::undefined();
  pluss::Series p_series = pluss::series::undefined();

  NegativeBinomial() = default;
  NegativeBinomial(double r_, double p_) : r(r_), p(p_) {}
  NegativeBinomial(const pluss::Series& r_series_, const pluss::Series& p_series_)
      : r_series(r_series_), p_series(p_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  NegativeBinomialFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Gamma {
  double shape = 1.0;
  double scale = 1.0;
  double location = 0.0;
  pluss::Series shape_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();
  pluss::Series location_series = pluss::series::undefined();

  Gamma() = default;
  Gamma(double shape_, double scale_, double location_)
      : shape(shape_), scale(scale_), location(location_) {}
  Gamma(const pluss::Series& shape_series_,
        const pluss::Series& scale_series_,
        const pluss::Series& location_series_)
      : shape_series(shape_series_),
        scale_series(scale_series_),
        location_series(location_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  GammaFit MLE(const pluss::Series& x) const;
};

struct CODEM_API ChiSquare {
  std::int64_t degrees_of_freedom = 1;
  pluss::Series degrees_of_freedom_series = pluss::series::undefined();

  ChiSquare() = default;
  explicit ChiSquare(std::int64_t degrees_of_freedom_) : degrees_of_freedom(degrees_of_freedom_) {}
  explicit ChiSquare(const pluss::Series& degrees_of_freedom_series_)
      : degrees_of_freedom_series(degrees_of_freedom_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API InverseGaussian {
  double theta = 1.0;
  double chi = 1.0;
  pluss::Series theta_series = pluss::series::undefined();
  pluss::Series chi_series = pluss::series::undefined();

  InverseGaussian() = default;
  InverseGaussian(double theta_, double chi_) : theta(theta_), chi(chi_) {}
  InverseGaussian(const pluss::Series& theta_series_, const pluss::Series& chi_series_)
      : theta_series(theta_series_), chi_series(chi_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  InverseGaussianFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Lognormal {
  double mean_log = 0.0;
  double stddev_log = 1.0;
  pluss::Series mean_log_series = pluss::series::undefined();
  pluss::Series stddev_log_series = pluss::series::undefined();

  Lognormal() = default;
  Lognormal(double mean_log_, double stddev_log_) : mean_log(mean_log_), stddev_log(stddev_log_) {}
  Lognormal(const pluss::Series& mean_log_series_, const pluss::Series& stddev_log_series_)
      : mean_log_series(mean_log_series_), stddev_log_series(stddev_log_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  LognormalFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Uniform {
  double low = 0.0;
  double high = 1.0;
  pluss::Series low_series = pluss::series::undefined();
  pluss::Series high_series = pluss::series::undefined();

  Uniform() = default;
  Uniform(double low_, double high_) : low(low_), high(high_) {}
  Uniform(const pluss::Series& low_series_, const pluss::Series& high_series_)
      : low_series(low_series_), high_series(high_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  UniformFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Exponential {
  double rate = 1.0;
  pluss::Series rate_series = pluss::series::undefined();

  Exponential() = default;
  explicit Exponential(double rate_) : rate(rate_) {}
  explicit Exponential(const pluss::Series& rate_series_) : rate_series(rate_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  ExponentialFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Laplace {
  double location = 0.0;
  double scale = 1.0;
  pluss::Series location_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();

  Laplace() = default;
  Laplace(double location_, double scale_) : location(location_), scale(scale_) {}
  Laplace(const pluss::Series& location_series_, const pluss::Series& scale_series_)
      : location_series(location_series_), scale_series(scale_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  LaplaceFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Logistic {
  double location = 0.0;
  double scale = 1.0;
  pluss::Series location_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();

  Logistic() = default;
  Logistic(double location_, double scale_) : location(location_), scale(scale_) {}
  Logistic(const pluss::Series& location_series_, const pluss::Series& scale_series_)
      : location_series(location_series_), scale_series(scale_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API Rayleigh {
  double sigma = 1.0;
  pluss::Series sigma_series = pluss::series::undefined();

  Rayleigh() = default;
  explicit Rayleigh(double sigma_) : sigma(sigma_) {}
  explicit Rayleigh(const pluss::Series& sigma_series_) : sigma_series(sigma_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  RayleighFit MLE(const pluss::Series& x) const;
};

struct CODEM_API NormalTrunc {
  double lower = -1.0;
  double upper = 1.0;
  double mean = 0.0;
  double stddev = 1.0;

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API StudentT {
  double degrees_of_freedom = 1.0;
  double location = 0.0;
  double scale = 1.0;
  pluss::Series degrees_of_freedom_series = pluss::series::undefined();
  pluss::Series location_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();

  StudentT() = default;
  StudentT(double degrees_of_freedom_, double location_, double scale_)
      : degrees_of_freedom(degrees_of_freedom_), location(location_), scale(scale_) {}
  StudentT(const pluss::Series& degrees_of_freedom_series_,
           const pluss::Series& location_series_,
           const pluss::Series& scale_series_)
      : degrees_of_freedom_series(degrees_of_freedom_series_),
        location_series(location_series_),
        scale_series(scale_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API FDistribution {
  double numerator_degrees_of_freedom = 1.0;
  double denominator_degrees_of_freedom = 1.0;

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API Weibull {
  double shape = 1.0;
  double scale = 1.0;
  pluss::Series shape_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();

  Weibull() = default;
  Weibull(double shape_, double scale_) : shape(shape_), scale(scale_) {}
  Weibull(const pluss::Series& shape_series_, const pluss::Series& scale_series_)
      : shape_series(shape_series_), scale_series(scale_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API Beta {
  double low = 0.0;
  double high = 1.0;
  double alpha = 1.0;
  double beta = 1.0;
  pluss::Series low_series = pluss::series::undefined();
  pluss::Series high_series = pluss::series::undefined();
  pluss::Series alpha_series = pluss::series::undefined();
  pluss::Series beta_series = pluss::series::undefined();

  Beta() = default;
  Beta(double low_, double high_, double alpha_, double beta_)
      : low(low_), high(high_), alpha(alpha_), beta(beta_) {}
  Beta(const pluss::Series& low_series_,
       const pluss::Series& high_series_,
       const pluss::Series& alpha_series_,
       const pluss::Series& beta_series_)
      : low_series(low_series_),
        high_series(high_series_),
        alpha_series(alpha_series_),
        beta_series(beta_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  BetaFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Skellam {
  double lambda1 = 1.0;
  double lambda2 = 1.0;

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
};

struct CODEM_API Nig {
  double alpha = 1.0;
  double beta = 0.0;
  double location = 0.0;
  double scale = 1.0;
  pluss::Series alpha_series = pluss::series::undefined();
  pluss::Series beta_series = pluss::series::undefined();
  pluss::Series location_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();

  Nig() = default;
  Nig(double alpha_, double beta_, double location_, double scale_)
      : alpha(alpha_), beta(beta_), location(location_), scale(scale_) {}
  Nig(const pluss::Series& alpha_series_,
      const pluss::Series& beta_series_,
      const pluss::Series& location_series_,
      const pluss::Series& scale_series_)
      : alpha_series(alpha_series_),
        beta_series(beta_series_),
        location_series(location_series_),
        scale_series(scale_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  NigFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Gh {
  double lambda = 1.0;
  double alpha = 2.0;
  double beta = 0.0;
  double location = 0.0;
  double scale = 1.0;
  pluss::Series lambda_series = pluss::series::undefined();
  pluss::Series alpha_series = pluss::series::undefined();
  pluss::Series beta_series = pluss::series::undefined();
  pluss::Series location_series = pluss::series::undefined();
  pluss::Series scale_series = pluss::series::undefined();

  Gh() = default;
  Gh(double lambda_, double alpha_, double beta_, double location_, double scale_)
      : lambda(lambda_), alpha(alpha_), beta(beta_), location(location_), scale(scale_) {}
  Gh(const pluss::Series& lambda_series_,
     const pluss::Series& alpha_series_,
     const pluss::Series& beta_series_,
     const pluss::Series& location_series_,
     const pluss::Series& scale_series_)
      : lambda_series(lambda_series_),
        alpha_series(alpha_series_),
        beta_series(beta_series_),
        location_series(location_series_),
        scale_series(scale_series_) {}

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0) const;
  pluss::Series sample_like(const pluss::Series& like, std::uint64_t seed = 0) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  GhFit MLE(const pluss::Series& x) const;
};

struct CODEM_API GaussianMixture {
  pluss::Series weights = pluss::series::undefined();
  pluss::Series means = pluss::series::undefined();
  pluss::Series stddevs = pluss::series::undefined();

  pluss::Series sample(std::span<const std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0,
                       bool use_sobol = false) const;
  pluss::Series sample(std::initializer_list<std::int64_t> sizes,
                       pluss::Config cfg = {},
                       std::uint64_t seed = 0,
                       bool use_sobol = false) const;
  pluss::Series sample_like(const pluss::Series& like,
                            std::uint64_t seed = 0,
                            bool use_sobol = false) const;
  pluss::Series pdf(const pluss::Series& x) const;
  pluss::Series logpdf(const pluss::Series& x) const;
  pluss::Series logcdf(const pluss::Series& x) const;
  pluss::Series cdf(const pluss::Series& x) const;
  pluss::Series cgf(const pluss::Series& t) const;
  pluss::Series cumulant(std::int64_t order) const;
  GaussianMixtureFit MLE(const pluss::Series& x) const;
};

} // namespace pluss::subsp::stats::distr
