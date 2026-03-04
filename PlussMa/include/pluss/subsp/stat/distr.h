#pragma once

#include "pluss/subsp/stat/distr_fwd.h"

namespace pluss::subsp::stat::distr {

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
};

} // namespace pluss::subsp::stat::distr
