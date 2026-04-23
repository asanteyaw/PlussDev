#pragma once

#include "pluss/codem/codem.h"
#include "pluss/export.h"

#include <cstdint>

namespace pluss::subsp::actuarial::loss_models {

struct AggregateMoments {
  pluss::codem::Series mean = pluss::codem::Series::undefined();
  pluss::codem::Series variance = pluss::codem::Series::undefined();
};

// Poisson frequency moments.
CODEM_API AggregateMoments poisson_frequency_moments(
    double lambda,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Negative-binomial frequency moments (failures-before-rth-success parameterization).
CODEM_API AggregateMoments negative_binomial_frequency_moments(
    double r,
    double p,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Lognormal severity moments.
CODEM_API AggregateMoments lognormal_severity_moments(
    double mean_log,
    double stddev_log,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Gamma severity moments.
CODEM_API AggregateMoments gamma_severity_moments(
    double shape,
    double scale,
    double location = 0.0,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Aggregate moments for S = sum_{i=1}^{N} X_i with independent N and X_i:
// E[S] = E[N]E[X], Var[S] = E[N]Var[X] + Var[N](E[X])^2.
CODEM_API AggregateMoments aggregate_moments(const pluss::codem::Series& frequency_mean,
                                             const pluss::codem::Series& frequency_variance,
                                             const pluss::codem::Series& severity_mean,
                                             const pluss::codem::Series& severity_variance);

// Compound-Poisson aggregate moments with scalar severity moments.
CODEM_API AggregateMoments compound_poisson_aggregate_moments(
    double lambda,
    double severity_mean,
    double severity_variance,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Compound-negative-binomial aggregate moments with scalar severity moments.
CODEM_API AggregateMoments compound_negative_binomial_aggregate_moments(
    double r,
    double p,
    double severity_mean,
    double severity_variance,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Poisson(lambda) pmf on k = 0..max_count.
CODEM_API pluss::codem::Series poisson_frequency_pmf(double lambda,
                                                     std::int64_t max_count,
                                                     pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Negative-binomial(r, p) pmf on k = 0..max_count (failures count support).
CODEM_API pluss::codem::Series negative_binomial_frequency_pmf(
    double r,
    double p,
    std::int64_t max_count,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Per-loss excess above attachment: (X - d)+.
CODEM_API pluss::codem::Series excess_loss(const pluss::codem::Series& losses,
                                           double attachment);

// Per-loss layer with attachment + finite limit: min((X - d)+, limit).
CODEM_API pluss::codem::Series limited_excess_loss(const pluss::codem::Series& losses,
                                                   double attachment,
                                                   double limit);

// Empirical expected layer payout E[min((X - d)+, limit)].
CODEM_API pluss::codem::Series layer_premium(const pluss::codem::Series& losses,
                                             double attachment,
                                             double limit);

// Parametric limited expected value E[min(X, cap)] under Lognormal(mean_log, stddev_log).
CODEM_API pluss::codem::Series lognormal_limited_expected_value(
    double mean_log,
    double stddev_log,
    double cap,
    std::int64_t quadrature_steps = 2048,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Parametric limited expected value E[min(X, cap)] under Gamma(shape, scale, location).
CODEM_API pluss::codem::Series gamma_limited_expected_value(
    double shape,
    double scale,
    double location,
    double cap,
    std::int64_t quadrature_steps = 2048,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Parametric layer premium E[min((X - d)+, limit)] under Lognormal severity.
CODEM_API pluss::codem::Series lognormal_layer_premium(
    double mean_log,
    double stddev_log,
    double attachment,
    double limit,
    std::int64_t quadrature_steps = 2048,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Parametric layer premium E[min((X - d)+, limit)] under Gamma severity.
CODEM_API pluss::codem::Series gamma_layer_premium(
    double shape,
    double scale,
    double location,
    double attachment,
    double limit,
    std::int64_t quadrature_steps = 2048,
    pluss::codem::Platform platform = pluss::codem::Platform::CPU);

// Empirical stop-loss premium E[(X - d)+] over a sample loss vector.
CODEM_API pluss::codem::Series stop_loss_premium(const pluss::codem::Series& losses,
                                                 double deductible);

} // namespace pluss::subsp::actuarial::loss_models
