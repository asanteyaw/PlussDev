#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/stats/rng.h"

#include <cstdint>
#include <initializer_list>
#include <span>

namespace pluss::subsp::stats::distr {

namespace dst {

enum class Kind : std::uint8_t {
  Alpha,
  Anglit,
  Arcsin,
  Bessel,
  Normal,
  Lognormal,
  Uniform,
  Exponential,
  Bernoulli,
  Beta,
  Binomial,
  Bradford,
  Burr,
  Cauchy,
  Chi,
  ChiSquare,
  ChiSquareNoncentral,
  Cosine,
  Erlang,
  ExtremeValue,
  F,
  FNoncentral,
  Laplace,
  Logistic,
  Geometric,
  Gamma,
  Gompertz,
  HalfCosine,
  HyperbolicSecant,
  Hypergeometric,
  InverseGaussian,
  Maxwell,
  Nakagami,
  NegativeBinomial,
  NormalTrunc,
  Planck,
  Poisson,
  Quadratic,
  Rademacher,
  StudentT,
  Triangular,
  Pareto,
  Lomax,
  Rayleigh,
  Rice,
  Semicircle,
  Skellam,
  TukeyLambda,
  UShape,
  VonMises,
  Wald,
  Weibull,
  Yule,
  Zeta,
  Gig,
  Gh,
  Nig,
};

inline constexpr Kind Alpha = Kind::Alpha;
inline constexpr Kind Anglit = Kind::Anglit;
inline constexpr Kind Arcsin = Kind::Arcsin;
inline constexpr Kind Bessel = Kind::Bessel;
inline constexpr Kind Normal = Kind::Normal;
inline constexpr Kind Lognormal = Kind::Lognormal;
inline constexpr Kind Uniform = Kind::Uniform;
inline constexpr Kind Exponential = Kind::Exponential;
inline constexpr Kind Bernoulli = Kind::Bernoulli;
inline constexpr Kind Beta = Kind::Beta;
inline constexpr Kind Binomial = Kind::Binomial;
inline constexpr Kind Bradford = Kind::Bradford;
inline constexpr Kind Burr = Kind::Burr;
inline constexpr Kind Cauchy = Kind::Cauchy;
inline constexpr Kind Chi = Kind::Chi;
inline constexpr Kind ChiSquare = Kind::ChiSquare;
inline constexpr Kind ChiSquareNoncentral = Kind::ChiSquareNoncentral;
inline constexpr Kind Cosine = Kind::Cosine;
inline constexpr Kind Erlang = Kind::Erlang;
inline constexpr Kind ExtremeValue = Kind::ExtremeValue;
inline constexpr Kind F = Kind::F;
inline constexpr Kind FNoncentral = Kind::FNoncentral;
inline constexpr Kind Laplace = Kind::Laplace;
inline constexpr Kind Logistic = Kind::Logistic;
inline constexpr Kind Geometric = Kind::Geometric;
inline constexpr Kind Gamma = Kind::Gamma;
inline constexpr Kind Gompertz = Kind::Gompertz;
inline constexpr Kind HalfCosine = Kind::HalfCosine;
inline constexpr Kind HyperbolicSecant = Kind::HyperbolicSecant;
inline constexpr Kind Hypergeometric = Kind::Hypergeometric;
inline constexpr Kind InverseGaussian = Kind::InverseGaussian;
inline constexpr Kind Maxwell = Kind::Maxwell;
inline constexpr Kind Nakagami = Kind::Nakagami;
inline constexpr Kind NegativeBinomial = Kind::NegativeBinomial;
inline constexpr Kind NormalTrunc = Kind::NormalTrunc;
inline constexpr Kind Planck = Kind::Planck;
inline constexpr Kind Poisson = Kind::Poisson;
inline constexpr Kind Quadratic = Kind::Quadratic;
inline constexpr Kind Rademacher = Kind::Rademacher;
inline constexpr Kind StudentT = Kind::StudentT;
inline constexpr Kind Triangular = Kind::Triangular;
inline constexpr Kind Pareto = Kind::Pareto;
inline constexpr Kind Lomax = Kind::Lomax;
inline constexpr Kind Rayleigh = Kind::Rayleigh;
inline constexpr Kind Rice = Kind::Rice;
inline constexpr Kind Semicircle = Kind::Semicircle;
inline constexpr Kind Skellam = Kind::Skellam;
inline constexpr Kind TukeyLambda = Kind::TukeyLambda;
inline constexpr Kind UShape = Kind::UShape;
inline constexpr Kind VonMises = Kind::VonMises;
inline constexpr Kind Wald = Kind::Wald;
inline constexpr Kind Weibull = Kind::Weibull;
inline constexpr Kind Yule = Kind::Yule;
inline constexpr Kind Zeta = Kind::Zeta;
inline constexpr Kind Gig = Kind::Gig;
inline constexpr Kind Gh = Kind::Gh;
inline constexpr Kind Nig = Kind::Nig;

} // namespace dst

CODEM_API bool supports(dst::Kind kind) noexcept;

CODEM_API pluss::codem::Series sample(dst::Kind kind,
                                      std::span<const double> params,
                                      std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0,
                                      rng::Method method = rng::Method::Pseudo);
CODEM_API pluss::codem::Series sample(dst::Kind kind,
                                      std::initializer_list<double> params,
                                      std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0,
                                      rng::Method method = rng::Method::Pseudo);
CODEM_API pluss::codem::Series sample(dst::Kind kind,
                                      std::initializer_list<double> params,
                                      std::initializer_list<std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0,
                                      rng::Method method = rng::Method::Pseudo);
CODEM_API pluss::codem::Series sample(dst::Kind kind,
                                      std::span<const std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0,
                                      rng::Method method = rng::Method::Pseudo);
CODEM_API pluss::codem::Series sample(dst::Kind kind,
                                      std::initializer_list<std::int64_t> sizes,
                                      pluss::codem::Config cfg = {},
                                      std::uint64_t seed = 0,
                                      rng::Method method = rng::Method::Pseudo);

CODEM_API pluss::codem::Series sample_like(dst::Kind kind,
                                           std::span<const double> params,
                                           const pluss::codem::Series& like,
                                           std::uint64_t seed = 0,
                                           rng::Method method = rng::Method::Pseudo);
CODEM_API pluss::codem::Series sample_like(dst::Kind kind,
                                           std::initializer_list<double> params,
                                           const pluss::codem::Series& like,
                                           std::uint64_t seed = 0,
                                           rng::Method method = rng::Method::Pseudo);
CODEM_API pluss::codem::Series sample_like(dst::Kind kind,
                                           const pluss::codem::Series& like,
                                           std::uint64_t seed = 0,
                                           rng::Method method = rng::Method::Pseudo);

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
CODEM_API pluss::codem::Series mixture_normal_logcdf(const pluss::codem::Series& x,
                                                     const pluss::codem::Series& weights,
                                                     const pluss::codem::Series& means,
                                                     const pluss::codem::Series& stddevs);

CODEM_API pluss::codem::Series pdf(dst::Kind kind,
                                   const pluss::codem::Series& x,
                                   std::span<const double> params = {});
CODEM_API pluss::codem::Series pdf(dst::Kind kind,
                                   const pluss::codem::Series& x,
                                   std::initializer_list<double> params);

CODEM_API pluss::codem::Series logpdf(dst::Kind kind,
                                      const pluss::codem::Series& x,
                                      std::span<const double> params = {});
CODEM_API pluss::codem::Series logpdf(dst::Kind kind,
                                      const pluss::codem::Series& x,
                                      std::initializer_list<double> params);

CODEM_API pluss::codem::Series cdf(dst::Kind kind,
                                   const pluss::codem::Series& x,
                                   std::span<const double> params = {});
CODEM_API pluss::codem::Series cdf(dst::Kind kind,
                                   const pluss::codem::Series& x,
                                   std::initializer_list<double> params);

CODEM_API pluss::codem::Series logcdf(dst::Kind kind,
                                      const pluss::codem::Series& x,
                                      std::span<const double> params = {});
CODEM_API pluss::codem::Series logcdf(dst::Kind kind,
                                      const pluss::codem::Series& x,
                                      std::initializer_list<double> params);

} // namespace pluss::subsp::stats::distr
