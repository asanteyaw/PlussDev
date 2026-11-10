<!-- generated from filtering.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Filtering

Estimating a hidden state from noisy observations, one date at a time. [`kalman_filter`](filtering/kalman_filter.md) is exact for linear Gaussian models; [`ekf_filter`](filtering/ekf_filter.md) and [`ukf_filter`](filtering/ukf_filter.md) extend it to smooth nonlinear ones; [`particle_filter`](filtering/particle_filter.md) represents any posterior by a weighted cloud, and [`sv_particle_filter`](filtering/sv_particle_filter.md) is a ready model for stochastic volatility. Each returns the filtered state, the innovations and the log-likelihood of the observations, which is what turns a filter into a tool for estimating parameters.

| Your model | Use |
|---|---|
| Linear dynamics, Gaussian noise | [`kalman_filter`](filtering/kalman_filter.md) with a [`KalmanModel`](filtering/KalmanModel.md) |
| Nonlinear, smooth, with Jacobians at hand | [`ekf_filter`](filtering/ekf_filter.md), [`EkfModel`](filtering/EkfModel.md) |
| Nonlinear, smooth, no Jacobians | [`ukf_filter`](filtering/ukf_filter.md), [`UkfModel`](filtering/UkfModel.md) |
| Non-Gaussian, jumps, heavy tails | [`particle_filter`](filtering/particle_filter.md) |
| A latent log-variance behind returns | [`sv_particle_filter`](filtering/sv_particle_filter.md) |

## The Surface

| Symbol | What it is |
|---|---|
| [`EkfModel`](filtering/EkfModel.md) | A nonlinear state-space model for the extended Kalman filter: the dynamics, the observation and their Jacobians. |
| [`KalmanModel`](filtering/KalmanModel.md) | A linear Gaussian state-space model: the matrices of the dynamics and the observation, the noise covariances and the initial state. |
| [`KalmanResult`](filtering/KalmanResult.md) | What the Kalman, extended and unscented filters return: the filtered and predicted states, the innovations, which data were used, and the log-likelihood. |
| [`ParticleFilterModel`](filtering/ParticleFilterModel.md) | A state-space model for the particle filter: how particles move, how likely an observation is under each, and the starting cloud. |
| [`ParticleFilterOptions`](filtering/ParticleFilterOptions.md) | Resampling and storage settings of a particle filter. |
| [`ParticleFilterResult`](filtering/ParticleFilterResult.md) | What a particle filter returns: the filtered mean, the weights, the cloud, the effective sample size and the log-likelihood estimate. |
| [`ParticleResampleMethod`](filtering/ParticleResampleMethod.md) | How a particle filter redraws the cloud when the weights have concentrated. |
| [`ParticleResampleTrigger`](filtering/ParticleResampleTrigger.md) | When a particle filter resamples: when the weights degrade, or at fixed intervals. |
| [`UkfModel`](filtering/UkfModel.md) | A nonlinear state-space model for the unscented Kalman filter: the dynamics and the observation, without Jacobians. |
| [`UkfOptions`](filtering/UkfOptions.md) | The spread and prior of the unscented filter's sigma points, and its numerical jitter. |
| [`ekf_filter`](filtering/ekf_filter.md) | The extended Kalman filter: the Kalman equations linearised around the current estimate. |
| [`kalman_filter`](filtering/kalman_filter.md) | The Kalman filter: the exact posterior of a linear Gaussian state-space model, date by date. |
| [`particle_filter`](filtering/particle_filter.md) | The bootstrap particle filter for any state-space model you can simulate and whose observation density you can evaluate. |
| [`sv_particle_filter`](filtering/sv_particle_filter.md) | A ready particle filter for the log-variance of a stochastic-volatility model, from a series of log returns. |
| [`ukf_filter`](filtering/ukf_filter.md) | The unscented Kalman filter: sigma points through the nonlinear model, with no Jacobians. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

filtering/EkfModel
filtering/KalmanModel
filtering/KalmanResult
filtering/ParticleFilterModel
filtering/ParticleFilterOptions
filtering/ParticleFilterResult
filtering/ParticleResampleMethod
filtering/ParticleResampleTrigger
filtering/UkfModel
filtering/UkfOptions
filtering/ekf_filter
filtering/kalman_filter
filtering/particle_filter
filtering/sv_particle_filter
filtering/ukf_filter
```
<!-- apiref:toctree:end -->
