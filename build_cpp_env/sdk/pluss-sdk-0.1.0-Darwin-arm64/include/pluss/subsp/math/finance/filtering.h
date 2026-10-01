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
  pluss::codem::Series transition = pluss::series::undefined();      // [n,n]
  pluss::codem::Series observation = pluss::series::undefined();     // [m,n]
  pluss::codem::Series process_cov = pluss::series::undefined();     // [n,n]
  pluss::codem::Series observation_cov = pluss::series::undefined(); // [m,m]
  pluss::codem::Series initial_mean = pluss::series::undefined();    // [n] or [n,1]
  pluss::codem::Series initial_cov = pluss::series::undefined();     // [n,n]
  pluss::codem::Series control = pluss::series::undefined();         // [T,k] optional
  pluss::codem::Series control_matrix = pluss::series::undefined();  // [n,k] optional
  double info_tol = 0.0;                                                     // ignore obs channels with forecast var <= info_tol
};

struct KalmanResult {
  pluss::codem::Series filtered_mean = pluss::series::undefined();   // [T,n]
  pluss::codem::Series filtered_cov = pluss::series::undefined();    // [T,n,n]
  pluss::codem::Series predicted_mean = pluss::series::undefined();  // [T,n]
  pluss::codem::Series predicted_cov = pluss::series::undefined();   // [T,n,n]
  pluss::codem::Series innovation = pluss::series::undefined();      // [T,m]
  pluss::codem::Series innovation_cov = pluss::series::undefined();  // [T,m,m]
  pluss::codem::Series data_used = pluss::series::undefined();       // [T,m], 1.0 if channel used in update else 0.0
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
  pluss::codem::Series process_cov = pluss::series::undefined();
  pluss::codem::Series observation_cov = pluss::series::undefined();
  pluss::codem::Series initial_mean = pluss::series::undefined();
  pluss::codem::Series initial_cov = pluss::series::undefined();
  double info_tol = 0.0; // ignore obs channels with forecast var <= info_tol
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
  pluss::codem::Series process_cov = pluss::series::undefined();
  pluss::codem::Series observation_cov = pluss::series::undefined();
  pluss::codem::Series initial_mean = pluss::series::undefined();
  pluss::codem::Series initial_cov = pluss::series::undefined();
  double info_tol = 0.0; // ignore obs channels with forecast var <= info_tol
  UkfOptions options{};
};

CODEM_API KalmanResult ukf_filter(const pluss::codem::Series& observations,
                                  const UkfModel& model,
                                  pluss::codem::Config cfg = {});

using ParticleTransitionFn = std::function<pluss::codem::Series(const pluss::codem::Series&, std::int64_t)>;
using ParticleLogLikFn = std::function<pluss::codem::Series(const pluss::codem::Series&, const pluss::codem::Series&, std::int64_t)>;

enum class ParticleResampleMethod : std::uint8_t {
  Systematic = 0,
  Multinomial = 1,
  Stratified = 2,
  Residual = 3,
};

enum class ParticleResampleTrigger : std::uint8_t {
  Ratio = 0,
  Interval = 1,
};

struct ParticleFilterOptions {
  double ess_threshold = 0.5; // as fraction of num_particles
  bool resample = true;
  bool store_particles = true;
  ParticleResampleMethod resample_method = ParticleResampleMethod::Systematic;
  ParticleResampleTrigger resample_trigger = ParticleResampleTrigger::Ratio;
  std::int64_t resample_interval = 1;
  std::uint64_t resample_seed = 0;
};

struct ParticleFilterModel {
  ParticleTransitionFn transition;
  ParticleLogLikFn loglik;
  pluss::codem::Series initial_particles = pluss::series::undefined(); // [N,n]
  pluss::codem::Series initial_weights = pluss::series::undefined();   // [N] optional
  ParticleFilterOptions options{};
};

struct ParticleFilterResult {
  pluss::codem::Series filtered_mean = pluss::series::undefined(); // [T,n]
  pluss::codem::Series particle_weights = pluss::series::undefined(); // [T,N]
  pluss::codem::Series particles = pluss::series::undefined(); // [T,N,n] if stored
  pluss::codem::Series ess = pluss::series::undefined(); // [T]
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
