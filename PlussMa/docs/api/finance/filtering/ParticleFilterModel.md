<!-- generated from ParticleFilterModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `filtering::ParticleFilterModel`

A state-space model for the particle filter: how particles move, how likely an observation is under each, and the starting cloud.

`transition(particles, t)` moves a cloud `[N, n]` one step and returns `[N, n]` (drawing its own noise, so the cloud is a sample of the next state); `loglik(particles, observation, t)` returns the log of the observation density for each particle, shape `[N]`; `initial_particles` is the starting cloud `[N, n]` (a sample of the prior), with optional `initial_weights` `[N]`; `options` ([`ParticleFilterOptions`](ParticleFilterOptions.md)) sets resampling and storage. Nothing is assumed about Gaussianity or linearity: any model you can simulate and whose observation density you can evaluate can be filtered.

## Fields

### `transition`

`filtering::ParticleTransitionFn`, default `empty`.

`Series (const Series &particles, std::int64_t t)`: `[N, n]` to `[N, n]`. Required.

### `loglik`

`filtering::ParticleLogLikFn`, default `empty`.

`Series (const Series &particles, const Series &observation, std::int64_t t)`: `[N]`. Required.

### `initial_particles`

`pluss::Series`, default `undefined`.

Starting cloud `[N, n]`, `N > 1`.

### `initial_weights`

`pluss::Series`, default `undefined`.

Optional starting weights `[N]`.

### `options`

`filtering::ParticleFilterOptions`, default `{}`.

Resampling and storage.

## Guidance

Passed to `particle_filter`.

## Practical Examples

### Example 1

The transition and likelihood of a local level. The filter's log-likelihood estimate is close to the exact Kalman value.

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
  out.print_defaults(PrintOptions{}.fixed(3));
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
  out.writeln("particle loglik", res.loglik, ", exact", exact.loglik);
}
```

Output

```text
particle loglik -29.703 , exact -29.556
```

### Example 2

The initial cloud: a start at the wrong place is forgotten within a few dates. The cloud is centred at 5 instead of 0.

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
  out.print_defaults(PrintOptions{}.fixed(3));
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
  model.initial_particles = pluss::series::randn({N, 1}, cfg, 5) + 5.0;
  const auto res = flt::particle_filter(y, model, cfg);
  out.writeln("filtered mean at dates 0, 5, 20, 100:", res.filtered_mean.select(0, 0).sum().item<double>(), res.filtered_mean.select(0, 5).sum().item<double>(), res.filtered_mean.select(0, 20).sum().item<double>(), res.filtered_mean.select(0, 100).sum().item<double>(), "(Kalman:", exact.filtered_mean.select(0, 100).sum().item<double>(), ")");
}
```

Output

```text
filtered mean at dates 0, 5, 20, 100: 1.537 0.307 -0.619 0.981 (Kalman: 0.987 )
```

### Example 3

Initial weights: giving the particles unequal weights biases the start; here all the weight is on the particles above zero.

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

  auto model = make_model({});
  model.initial_weights = (model.initial_particles.reshape({N}) > 0.0).to(pluss::ScalarType::Float64);
  const auto res = flt::particle_filter(y, model, cfg);
  out.writeln("filtered mean at date 0 with weights on the positive half:", res.filtered_mean.select(0, 0).sum().item<double>(), " (no weights:", flt::particle_filter(y, make_model({}), cfg).filtered_mean.select(0, 0).sum().item<double>(), ")");
}
```

Output

```text
filtered mean at date 0 with weights on the positive half: 0.1411  (no weights: -0.0030 )
```

### Example 4

A non-Gaussian likelihood: Student-t observation noise with 3 degrees of freedom. The Kalman filter has no such model; the particle filter handles it by changing `loglik`. These observations have no outliers, so the two filters agree closely here; the heavy-tailed likelihood pays off when some observations are wild.

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

  auto model = make_model({});
  model.loglik = [&](const Series &particles, const Series &observation, std::int64_t) {
    const Series z = (observation.reshape({1, 1}) - particles) / std::sqrt(r);
    return ((z.square() / 3.0 + 1.0).log() * -2.0).reshape({N});   // log density up to a constant, 3 degrees of freedom
  };
  const auto res = flt::particle_filter(y, model, cfg);
  out.writeln("filtered level at the last date:", res.filtered_mean.select(0, T - 1).sum().item<double>(), ", Gaussian-noise filter:", exact.filtered_mean.select(0, T - 1).sum().item<double>());
}
```

Output

```text
filtered level at the last date: 2.3423 , Gaussian-noise filter: 2.3359
```

### Example 5

A missing callback is refused.

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
  model.loglik = nullptr;
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
subsp::math::finance::filtering::particle_filter: loglik function missing
```

### Example 6

The options live in the model.

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
  out.print_defaults(PrintOptions{}.fixed(1));
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
  model.options.ess_threshold = 0.9;
  out.writeln("minimum ESS with threshold 0.9:", flt::particle_filter(y, model, cfg).ess.amin().item<double>());
}
```

Output

```text
minimum ESS with threshold 0.9: 78.2
```

## See Also

- [`particle_filter`](particle_filter.md): uses it
- [`ParticleFilterOptions`](ParticleFilterOptions.md): the options

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="KalmanResult.html">
    <span class="label">Previous</span>
    <span class="title">KalmanResult</span>
  </a>
  <a class="next-link" href="ParticleFilterOptions.html">
    <span class="label">Next</span>
    <span class="title">ParticleFilterOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
