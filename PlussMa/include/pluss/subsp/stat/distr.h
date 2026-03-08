#pragma once

#include "pluss/subsp/stat/distr_fwd.h"

namespace pluss::subsp::stat::distr {

struct Normal;
struct Bernoulli;
struct Binomial;
struct Geometric;
struct Lognormal;
struct Uniform;
struct Exponential;
struct Rayleigh;
struct Poisson;

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
};

struct CODEM_API Normal {
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
  NormalFit MLE(const pluss::Series& x) const;
};

struct CODEM_API Bernoulli {
  double p = 0.5;

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

struct CODEM_API Gamma {
  double shape = 1.0;
  double scale = 1.0;
  double location = 0.0;

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

struct CODEM_API ChiSquare {
  std::int64_t degrees_of_freedom = 1;

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

struct CODEM_API Lognormal {
  double mean_log = 0.0;
  double stddev_log = 1.0;

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

struct CODEM_API Logistic {
  double location = 0.0;
  double scale = 1.0;

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

struct CODEM_API Gh {
  double lambda = 1.0;
  double alpha = 2.0;
  double beta = 0.0;
  double location = 0.0;
  double scale = 1.0;

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

struct CODEM_API GaussianMixture {
  pluss::Series weights;
  pluss::Series means;
  pluss::Series stddevs;

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
};

} // namespace pluss::subsp::stat::distr
