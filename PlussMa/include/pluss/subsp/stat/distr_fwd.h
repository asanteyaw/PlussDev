#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"

#include <cstdint>
#include <initializer_list>
#include <span>
#include <string_view>

namespace pluss::subsp::stat::distr {

CODEM_API bool supports(std::string_view name) noexcept;

CODEM_API pluss::codem::Series sample(std::string_view name,
                                      std::span<const double> params,
                                      std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0);
CODEM_API pluss::codem::Series sample(std::string_view name,
                                      std::initializer_list<double> params,
                                      std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0);
CODEM_API pluss::codem::Series sample(std::string_view name,
                                      std::initializer_list<double> params,
                                      std::initializer_list<std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0);
CODEM_API pluss::codem::Series sample(std::string_view name,
                                      std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0);
CODEM_API pluss::codem::Series sample(std::string_view name,
                                      std::initializer_list<std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0);

CODEM_API pluss::codem::Series sample_like(std::string_view name,
                                           std::span<const double> params,
                                           const pluss::codem::Series& like,
                                           std::uint64_t seed = 0);
CODEM_API pluss::codem::Series sample_like(std::string_view name,
                                           std::initializer_list<double> params,
                                           const pluss::codem::Series& like,
                                           std::uint64_t seed = 0);
CODEM_API pluss::codem::Series sample_like(std::string_view name,
                                           const pluss::codem::Series& like,
                                           std::uint64_t seed = 0);

CODEM_API pluss::codem::Series mixture_normal(const pluss::codem::Series& weights,
                                              const pluss::codem::Series& means,
                                              const pluss::codem::Series& stddevs,
                                              std::span<const std::int64_t> sizes,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0,
                                              bool use_sobol = false);
CODEM_API pluss::codem::Series mixture_normal(const pluss::codem::Series& weights,
                                              const pluss::codem::Series& means,
                                              const pluss::codem::Series& stddevs,
                                              std::initializer_list<std::int64_t> sizes,
                                              pluss::codem::Config cfg = {},
                                              std::uint64_t seed = 0,
                                              bool use_sobol = false);
CODEM_API pluss::codem::Series mixture_normal_pdf(const pluss::codem::Series& x,
                                                  const pluss::codem::Series& weights,
                                                  const pluss::codem::Series& means,
                                                  const pluss::codem::Series& stddevs);
CODEM_API pluss::codem::Series mixture_normal_logpdf(const pluss::codem::Series& x,
                                                     const pluss::codem::Series& weights,
                                                     const pluss::codem::Series& means,
                                                     const pluss::codem::Series& stddevs);
CODEM_API pluss::codem::Series mixture_normal_cdf(const pluss::codem::Series& x,
                                                  const pluss::codem::Series& weights,
                                                  const pluss::codem::Series& means,
                                                  const pluss::codem::Series& stddevs);

CODEM_API pluss::codem::Series pdf(std::string_view name,
                                   const pluss::codem::Series& x,
                                   std::span<const double> params = {});
CODEM_API pluss::codem::Series pdf(std::string_view name,
                                   const pluss::codem::Series& x,
                                   std::initializer_list<double> params);

CODEM_API pluss::codem::Series logpdf(std::string_view name,
                                      const pluss::codem::Series& x,
                                      std::span<const double> params = {});
CODEM_API pluss::codem::Series logpdf(std::string_view name,
                                      const pluss::codem::Series& x,
                                      std::initializer_list<double> params);

CODEM_API pluss::codem::Series cdf(std::string_view name,
                                   const pluss::codem::Series& x,
                                   std::span<const double> params = {});
CODEM_API pluss::codem::Series cdf(std::string_view name,
                                   const pluss::codem::Series& x,
                                   std::initializer_list<double> params);

} // namespace pluss::subsp::stat::distr
