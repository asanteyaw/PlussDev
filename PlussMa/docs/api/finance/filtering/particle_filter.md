<!-- generated from particle_filter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `filtering::particle_filter`

The bootstrap particle filter for any state-space model you can simulate and whose observation density you can evaluate.

## Signature

```cpp
filtering::ParticleFilterResult filtering::particle_filter(const pluss::Series &observations, const filtering::ParticleFilterModel &model, pluss::Config cfg = {});
```

## What It Does

At each date the cloud is moved by `model.transition`, each particle's weight is multiplied by the observation density `exp(model.loglik)`, the weights are normalised, and the cloud is resampled when its effective sample size falls ([`ParticleFilterOptions`](ParticleFilterOptions.md)). The filtered mean, the weights, the effective sample size and an estimate of the log-likelihood are returned in a [`ParticleFilterResult`](ParticleFilterResult.md). The estimate converges to the exact posterior as the number of particles grows, at rate `1/√N`, and the log-likelihood estimate is unbiased on the likelihood scale. Use a seed fixed across evaluations when maximising the likelihood so that the function is smooth.

## Parameters

`const pluss::Series &observations`
: `[T]` or `[T, m]`.

`const filtering::ParticleFilterModel &model`
: Transition, likelihood, initial cloud, options.

`pluss::Config cfg`
: Device.

## Returns

A [`ParticleFilterResult`](ParticleFilterResult.md).

## Errors

- Missing callbacks, mis-shaped clouds or fewer than two particles throw (`pluss::Error`).

## Practical Examples

### Example 1

A local level where the exact answer is known. The particle filter recovers the Kalman filtered mean.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 200, N = 2000;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::full({1, 1}, r, cfg);
  km.initial_mean = pluss::series::zeros({1}, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  const auto exact = flt::kalman_filter(y, km, cfg);
  auto make_model = [&](flt::ParticleFilterOptions options) {
    flt::ParticleFilterModel model;
    model.initial_particles = pluss::series::randn({N, 1}, cfg, 5);
    model.options = options;
    model.transition = [&](const Series &particles, std::int64_t t) { return particles + pluss::series::randn({N, 1}, cfg, 1000 + t) * std::sqrt(q); };
    model.loglik = [&](const Series &particles, const Series &observation, std::int64_t) {
      const Series residual = observation.reshape({1, 1}) - particles;
      return ((residual.square() / (2.0 * r)) * -1.0 - 0.5 * std::log(2.0 * M_PI * r)).reshape({N});
    };
    return model;
  };

  const auto res = flt::particle_filter(y, make_model({}), cfg);
  out.writeln("rms gap between particle and Kalman means:", (res.filtered_mean.reshape({T}) - exact.filtered_mean.reshape({T})).square().mean().sqrt().item<double>());
  out.writeln("loglik", res.loglik, "exact", exact.loglik);
}
```

Output

```text
rms gap between particle and Kalman means: 0.0045
loglik -29.7028 exact -29.5565
```

### Example 2

More particles, smaller gap: the gap to the Kalman filter falls roughly like `1/√N`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 200, N = 2000;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::full({1, 1}, r, cfg);
  km.initial_mean = pluss::series::zeros({1}, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  const auto exact = flt::kalman_filter(y, km, cfg);
  auto make_model = [&](flt::ParticleFilterOptions options) {
    flt::ParticleFilterModel model;
    model.initial_particles = pluss::series::randn({N, 1}, cfg, 5);
    model.options = options;
    model.transition = [&](const Series &particles, std::int64_t t) { return particles + pluss::series::randn({N, 1}, cfg, 1000 + t) * std::sqrt(q); };
    model.loglik = [&](const Series &particles, const Series &observation, std::int64_t) {
      const Series residual = observation.reshape({1, 1}) - particles;
      return ((residual.square() / (2.0 * r)) * -1.0 - 0.5 * std::log(2.0 * M_PI * r)).reshape({N});
    };
    return model;
  };

  for (const std::int64_t n : {200, 2000, 20000}) {
    flt::ParticleFilterModel model;
    model.initial_particles = pluss::series::randn({n, 1}, cfg, 5);
    model.transition = [&](const Series &p, std::int64_t t) { return p + pluss::series::randn({n, 1}, cfg, 1000 + t) * std::sqrt(q); };
    model.loglik = [&](const Series &p, const Series &o, std::int64_t) { return (((o.reshape({1, 1}) - p).square() / (2.0 * r)) * -1.0 - 0.5 * std::log(2.0 * M_PI * r)).reshape({n}); };
    const auto res = flt::particle_filter(y, model, cfg);
    out.writeln("N =", n, ": rms gap", (res.filtered_mean.reshape({T}) - exact.filtered_mean.reshape({T})).square().mean().sqrt().item<double>());
  }
}
```

Output

```text
N = 200 : rms gap 0.01304
N = 2000 : rms gap 0.00449
N = 20000 : rms gap 0.00143
```

### Example 3

A nonlinear, non-Gaussian model: a stochastic-volatility-like state with squared-exponential observation noise. Particle filtering recovers a hidden log-volatility the Kalman filter cannot.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <vector>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 300, N = 3000;
  Series h = pluss::series::zeros({T}, cfg);
  std::vector<double> hv(T), rv(T);
  const std::vector<double> e1 = pluss::series::randn({T}, cfg, 1).items<double>(), e2 = pluss::series::randn({T}, cfg, 2).items<double>();
  double state = 0.0;
  for (int t = 0; t < T; ++t) { state = 0.95 * state + 0.25 * e1[t]; hv[t] = state; rv[t] = std::exp(0.5 * state) * 0.01 * e2[t]; }
  const Series returns = pluss::make_series(rv);
  flt::ParticleFilterModel model;
  model.initial_particles = pluss::series::randn({N, 1}, cfg, 5) * 0.8;
  model.transition = [&](const Series &p, std::int64_t t) { return p * 0.95 + pluss::series::randn({N, 1}, cfg, 100 + t) * 0.25; };
  model.loglik = [&](const Series &p, const Series &o, std::int64_t) {
    const Series var = p.exp() * 1e-4;
    return (var.log() * -0.5 - o.reshape({1, 1}).square() / (var * 2.0)).reshape({N});
  };
  const auto res = flt::particle_filter(returns, model, cfg);
  const Series truth = pluss::make_series(hv);
  const Series est = res.filtered_mean.reshape({T});
  out.writeln("correlation of the filtered log-variance with the truth:", ((est - est.mean()) * (truth - truth.mean())).mean().item<double>() / (est.std().item<double>() * truth.std().item<double>()));
}
```

Output

```text
correlation of the filtered log-variance with the truth: 0.755
```

### Example 4

Invalid input: fewer than two particles are refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 200, N = 2000;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::full({1, 1}, r, cfg);
  km.initial_mean = pluss::series::zeros({1}, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  const auto exact = flt::kalman_filter(y, km, cfg);
  auto make_model = [&](flt::ParticleFilterOptions options) {
    flt::ParticleFilterModel model;
    model.initial_particles = pluss::series::randn({N, 1}, cfg, 5);
    model.options = options;
    model.transition = [&](const Series &particles, std::int64_t t) { return particles + pluss::series::randn({N, 1}, cfg, 1000 + t) * std::sqrt(q); };
    model.loglik = [&](const Series &particles, const Series &observation, std::int64_t) {
      const Series residual = observation.reshape({1, 1}) - particles;
      return ((residual.square() / (2.0 * r)) * -1.0 - 0.5 * std::log(2.0 * M_PI * r)).reshape({N});
    };
    return model;
  };

  auto model = make_model({});
  model.initial_particles = pluss::series::randn({1, 1}, cfg, 5);
  try {
    (void)(flt::particle_filter(y, model, cfg));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::filtering::particle_filter: initial_particles must have N>1 and n>0
```

## See Also

- [`ParticleFilterModel`](ParticleFilterModel.md): the model
- [`kalman_filter`](kalman_filter.md): the exact filter for linear Gaussian models

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kalman_filter.html">
    <span class="label">Previous</span>
    <span class="title">kalman_filter</span>
  </a>
  <a class="next-link" href="sv_particle_filter.html">
    <span class="label">Next</span>
    <span class="title">sv_particle_filter</span>
  </a>
</div>
<!-- apiref:nav:end -->
