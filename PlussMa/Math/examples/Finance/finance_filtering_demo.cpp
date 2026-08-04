#include <pluss/ma.h>

#include <cstdint>
#include <cmath>
#include <iostream>

int main() {
  namespace flt = pluss::subsp::math::finance::filtering;
  namespace vol = pluss::subsp::math::finance::volatility;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const std::int64_t t_obs = 40;
  const auto y_linear = pluss::series::full({t_obs}, 1.2, cfg);
  const auto y_nonlinear = pluss::series::full({t_obs}, 1.0, cfg);

  // KF
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  km.observation_cov = pluss::series::full({1, 1}, 0.04, cfg);
  km.initial_mean = pluss::series::full({1}, 0.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  km.info_tol = 0.0;

  const auto kf = flt::kalman_filter(y_linear, km, cfg);

  std::cout << "Kalman filter\n";
  std::cout << "filtered_mean=" << kf.filtered_mean << "\n";
  std::cout << "data_used=" << kf.data_used << "\n";
  std::cout << "innovation=" << kf.innovation << "\n";
  std::cout << "loglik=" << kf.loglik << "\n";

  // EKF
  flt::EkfModel ekm;
  ekm.transition = [](const pluss::Series& x, std::int64_t) { return x.mul(0.95); };
  ekm.observation = [](const pluss::Series& x, std::int64_t) { return x.square(); };
  ekm.transition_jacobian = [cfg](const pluss::Series&, std::int64_t) { return pluss::series::full({1, 1}, 0.95, cfg); };
  ekm.observation_jacobian = [](const pluss::Series& x, std::int64_t) { return x.mul(2.0).reshape({1, 1}); };
  ekm.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  ekm.observation_cov = pluss::series::full({1, 1}, 0.05, cfg);
  ekm.initial_mean = pluss::series::full({1}, 1.0, cfg);
  ekm.initial_cov = pluss::series::full({1, 1}, 0.5, cfg);

  const auto ekf = flt::ekf_filter(y_nonlinear, ekm, cfg);
  std::cout << "\nExtended Kalman filter\n";
  std::cout << "filtered_mean=" << ekf.filtered_mean << "\n";
  std::cout << "data_used=" << ekf.data_used << "\n";
  std::cout << "loglik=" << ekf.loglik << "\n";

  // UKF
  flt::UkfModel ukm;
  ukm.transition = [](const pluss::Series& x, std::int64_t) { return x.mul(0.95); };
  ukm.observation = [](const pluss::Series& x, std::int64_t) { return x.square(); };
  ukm.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  ukm.observation_cov = pluss::series::full({1, 1}, 0.05, cfg);
  ukm.initial_mean = pluss::series::full({1}, 1.0, cfg);
  ukm.initial_cov = pluss::series::full({1, 1}, 0.5, cfg);
  ukm.options.alpha = 1e-3;
  ukm.options.beta = 2.0;
  ukm.options.kappa = 0.0;
  ukm.options.jitter = 1e-10;

  const auto ukf = flt::ukf_filter(y_nonlinear, ukm, cfg);
  std::cout << "\nUnscented Kalman filter\n";
  std::cout << "filtered_mean=" << ukf.filtered_mean << "\n";
  std::cout << "data_used=" << ukf.data_used << "\n";
  std::cout << "loglik=" << ukf.loglik << "\n";

  // PF
  flt::ParticleFilterModel pm;
  pm.initial_particles = pluss::series::randn({800, 1}, cfg, 20260711ULL);
  pm.options.ess_threshold = 0.6;
  pm.options.resample = true;
  pm.options.store_particles = true;
  pm.options.resample_method = flt::ParticleResampleMethod::Stratified;
  pm.options.resample_trigger = flt::ParticleResampleTrigger::Interval;
  pm.options.resample_interval = 2;
  pm.options.resample_seed = 20260714ULL;

  pm.transition = [=](const pluss::Series& particles, std::int64_t t) {
    const auto eps = pluss::series::randn({particles.sizes()[0], particles.sizes()[1]}, cfg,
                                  0xBEEFULL + static_cast<std::uint64_t>(t + 1));
    return particles.mul(0.98).add(eps.mul(0.12));
  };

  pm.loglik = [](const pluss::Series& particles, const pluss::Series& obs, std::int64_t) {
    const std::int64_t n = particles.sizes()[0];
    const double var = 0.15 * 0.15;
    const double log2pi = 1.8378770664093453;
    const auto resid = obs.reshape({1, 1}).sub(particles);
    return resid.square().div(var).add(log2pi + std::log(var)).mul(-0.5).reshape({n});
  };

  const auto pf = flt::particle_filter(pluss::series::full({t_obs}, 0.0, cfg), pm, cfg);

  std::cout << "\nParticle filter\n";
  std::cout << "filtered_mean=" << pf.filtered_mean << "\n";
  std::cout << "particle_weights=" << pf.particle_weights << "\n";
  std::cout << "ess=" << pf.ess << "\n";
  std::cout << "loglik=" << pf.loglik << "\n";

  vol::SvParams sp;
  sp.mu = 0.0;
  sp.logvar_mean = -3.2;
  sp.phi = 0.97;
  sp.vol_of_logvar = 0.24;
  sp.rho = -0.55;
  sp.logvar0 = -3.1;

  const auto log_returns = pluss::series::randn({50}, cfg, 20260712ULL).mul(0.02).add(0.0002);
  const auto svf = flt::sv_particle_filter(log_returns, sp, 1000, 1.0 / 252.0, 0.6, cfg, 20260713ULL);

  std::cout << "\nSV particle filter\n";
  std::cout << "filtered_mean=" << svf.filtered_mean << "\n";
  std::cout << "ess=" << svf.ess << "\n";
  std::cout << "loglik=" << svf.loglik << "\n";
  return 0;
}
