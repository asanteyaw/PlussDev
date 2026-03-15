#pragma once

#include "pluss/codem/codem.h"
#include "pluss/config.h"
#include "pluss/export.h"
#include "pluss/subsp/math/finance/volatility.h"

#include <cstdint>
#include <functional>

namespace pluss::subsp::math::finance::filtering {

using StateFn = std::function<pluss::codem::Series(const pluss::codem::Series&, std::int64_t)>;
using ObsFn = std::function<pluss::codem::Series(const pluss::codem::Series&, std::int64_t)>;
using JacobianFn = std::function<pluss::codem::Series(const pluss::codem::Series&, std::int64_t)>;

struct KalmanModel {
  pluss::codem::Series transition = pluss::codem::Series::undefined();      // [n,n]
  pluss::codem::Series observation = pluss::codem::Series::undefined();     // [m,n]
  pluss::codem::Series process_cov = pluss::codem::Series::undefined();     // [n,n]
  pluss::codem::Series observation_cov = pluss::codem::Series::undefined(); // [m,m]
  pluss::codem::Series initial_mean = pluss::codem::Series::undefined();    // [n] or [n,1]
  pluss::codem::Series initial_cov = pluss::codem::Series::undefined();     // [n,n]
  pluss::codem::Series control = pluss::codem::Series::undefined();         // [T,k] optional
  pluss::codem::Series control_matrix = pluss::codem::Series::undefined();  // [n,k] optional
};

struct KalmanResult {
  pluss::codem::Series filtered_mean = pluss::codem::Series::undefined();   // [T,n]
  pluss::codem::Series filtered_cov = pluss::codem::Series::undefined();    // [T,n,n]
  pluss::codem::Series predicted_mean = pluss::codem::Series::undefined();  // [T,n]
  pluss::codem::Series predicted_cov = pluss::codem::Series::undefined();   // [T,n,n]
  pluss::codem::Series innovation = pluss::codem::Series::undefined();      // [T,m]
  pluss::codem::Series innovation_cov = pluss::codem::Series::undefined();  // [T,m,m]
  double loglik = 0.0;
};

CODEM_API KalmanResult kalman_filter(const pluss::codem::Series& observations,
                                     const KalmanModel& model,
                                     pluss::codem::Config cfg = {});

struct EkfModel {
  StateFn transition;
  ObsFn observation;
  JacobianFn transition_jacobian;
  JacobianFn observation_jacobian;
  pluss::codem::Series process_cov = pluss::codem::Series::undefined();
  pluss::codem::Series observation_cov = pluss::codem::Series::undefined();
  pluss::codem::Series initial_mean = pluss::codem::Series::undefined();
  pluss::codem::Series initial_cov = pluss::codem::Series::undefined();
};

CODEM_API KalmanResult ekf_filter(const pluss::codem::Series& observations,
                                  const EkfModel& model,
                                  pluss::codem::Config cfg = {});

struct UkfOptions {
  double alpha = 1e-3;
  double beta = 2.0;
  double kappa = 0.0;
  double jitter = 1e-10;
};

struct UkfModel {
  StateFn transition;
  ObsFn observation;
  pluss::codem::Series process_cov = pluss::codem::Series::undefined();
  pluss::codem::Series observation_cov = pluss::codem::Series::undefined();
  pluss::codem::Series initial_mean = pluss::codem::Series::undefined();
  pluss::codem::Series initial_cov = pluss::codem::Series::undefined();
  UkfOptions options{};
};

CODEM_API KalmanResult ukf_filter(const pluss::codem::Series& observations,
                                  const UkfModel& model,
                                  pluss::codem::Config cfg = {});

using ParticleTransitionFn = std::function<pluss::codem::Series(const pluss::codem::Series&, std::int64_t)>;
using ParticleLogLikFn = std::function<pluss::codem::Series(const pluss::codem::Series&, const pluss::codem::Series&, std::int64_t)>;

struct ParticleFilterOptions {
  double ess_threshold = 0.5; // as fraction of num_particles
  bool resample = true;
  bool store_particles = true;
};

struct ParticleFilterModel {
  ParticleTransitionFn transition;
  ParticleLogLikFn loglik;
  pluss::codem::Series initial_particles = pluss::codem::Series::undefined(); // [N,n]
  pluss::codem::Series initial_weights = pluss::codem::Series::undefined();   // [N] optional
  ParticleFilterOptions options{};
};

struct ParticleFilterResult {
  pluss::codem::Series filtered_mean = pluss::codem::Series::undefined(); // [T,n]
  pluss::codem::Series particle_weights = pluss::codem::Series::undefined(); // [T,N]
  pluss::codem::Series particles = pluss::codem::Series::undefined(); // [T,N,n] if stored
  pluss::codem::Series ess = pluss::codem::Series::undefined(); // [T]
  double loglik = 0.0;
};

CODEM_API ParticleFilterResult particle_filter(const pluss::codem::Series& observations,
                                               const ParticleFilterModel& model,
                                               pluss::codem::Config cfg = {});

CODEM_API ParticleFilterResult sv_particle_filter(const pluss::codem::Series& log_returns,
                                                  const volatility::SvParams& params,
                                                  std::int64_t num_particles = 4000,
                                                  double dt = 1.0,
                                                  double ess_threshold = 0.5,
                                                  pluss::codem::Config cfg = {},
                                                  std::uint64_t seed = 0);

} // namespace pluss::subsp::math::finance::filtering
