<!-- generated from PathOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::PathOptions`

Options of the path-variation operators: whether to keep the running value and which axis is the path.

`PathOptions` is the argument of [`quadratic_variation`](quadratic_variation.md) and [`covariation`](covariation.md). A path functional is evaluated on arrays that hold the samples of a path along one axis; these two fields say what to return and where the samples are.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `cumulative` | `bool` | `false` | Return the running value at every sample (starting at 0) instead of the terminal value. |
| `path_axis` | `std::int64_t` | `-1` | The axis of the arguments that indexes the samples of one path; negative counts from the end. Every other axis is an independent path. |

## Guidance

Leave the defaults for a single terminal value on the last axis. Use `cumulative` to plot or test a running quantity, and `path_axis` when the sampling time runs along a different axis than the last.

## Practical Examples

### Example 1

`cumulative`. By default the whole path is summed to one number; with `cumulative = true` the running value at every sample is returned (starting at 0), and its last entry is the total.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const pluss::Series total = stochastic::quadratic_variation({.cumulative = false})(integrand)({times, path.w});
  const pluss::Series running = stochastic::quadratic_variation({.cumulative = true})(integrand)({times, path.w});
  out.writeln("total: shape", total.sizes(), " value", total.item<double>());
  out.writeln("running: shape", running.sizes(), " first", running.select(0, 0).item<double>(), " last", running.select(0, steps).item<double>());
}
```

Output

```text
total: shape []  value 1.030136
running: shape [2001]  first 0.000000  last 1.030136
```

### Example 2

`path_axis`. Arrays of paths can store the samples along any axis. With the path on the last axis (default `-1`) three paths are `[3, N+1]`; with `path_axis = 0` they are `[N+1, 3]`. Both give three values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const pluss::Series t3 = times.unsqueeze(0).repeat({3, 1}), w3 = path.w.unsqueeze(0).repeat({3, 1});
  out.writeln("[3, N+1], default axis:", stochastic::quadratic_variation()(integrand)({t3, w3}).items<double>());
  const pluss::Series t3t = times.unsqueeze(1).repeat({1, 3}), w3t = path.w.unsqueeze(1).repeat({1, 3});
  out.writeln("[N+1, 3], path_axis 0: ", stochastic::quadratic_variation({.path_axis = 0})(integrand)({t3t, w3t}).items<double>());
}
```

Output

```text
[3, N+1], default axis: [1.030136, 1.030136, 1.030136]
[N+1, 3], path_axis 0:  [1.030136, 1.030136, 1.030136]
```

### Example 3

Both fields apply to `covariation` too: the running covariation of `W` with `W + t`, which has the same increments' products as `[W]` plus a negligible smooth part.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::int64_t steps = 2000;
  const sde::WienerPath path = sde::simulate_wiener_path(pluss::make_series(0.0), 0.0, 1.0, steps, 7);
  const pluss::Series times = pluss::make_series(path.times, {steps + 1});
  const math::Function integrand = math::function({"t", "W"}, [](math::Function, math::Function w) { return w; });
  const double w_final = path.w.select(0, steps).item<double>();
  const double qv = stochastic::quadratic_variation()(integrand)({times, path.w}).item<double>();

  const math::Function shifted = math::function({"t", "W"}, [](math::Function t, math::Function w) { return w + t; });
  const pluss::Series running = stochastic::covariation(shifted, {.cumulative = true})(integrand)({times, path.w});
  out.writeln("running covariation: shape", running.sizes(), " last", running.select(0, steps).item<double>(), "  [W] =", qv);
}
```

Output

```text
running covariation: shape [2001]  last 1.030267   [W] = 1.030136
```

## See Also

- [`quadratic_variation`](quadratic_variation.md): uses these options
- [`covariation`](covariation.md): uses these options
- [`IntegralOptions`](IntegralOptions.md): adds convention and θ

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IntegralOptions.html">
    <span class="label">Previous</span>
    <span class="title">IntegralOptions</span>
  </a>
  <a class="next-link" href="convention_change.html">
    <span class="label">Next</span>
    <span class="title">convention_change</span>
  </a>
</div>
<!-- apiref:nav:end -->
