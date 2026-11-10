<!-- generated from StochasticIntegralResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::StochasticIntegralResult`

The value of a stochastic integral together with the realised quadratic variation of its path and any warnings.

`value` has the batch shape of the path. `realized_quadratic_variation` is `Σ (ΔW)²` for that path: it is what turns the Itô-versus-Stratonovich gap from a mystery into an identity (the two values differ by half of it, for the integrand `f = W`). `quadratic_variation_ratio` is its batch mean divided by the elapsed time, about 1 for a standard Wiener path; when it is further from 1 than sampling error allows, a message is added to `warnings` saying that this is not a standard Wiener path (another volatility, a smooth path, or jumps).

## Fields

### `value`

`pluss::Series`, default `undefined`.

The integral, shaped like the batch of the path.

### `realized_quadratic_variation`

`pluss::Series`, default `undefined`.

`Σ (ΔW)²` along the same path, per batch entry.

### `quadratic_variation_ratio`

`double`, default `1.0`.

Batch mean of the realised quadratic variation over `t1 − t0`; about 1 for a standard Wiener path.

### `warnings`

`std::vector<std::string>`, default `{}`.

Notes that do not stop the calculation but change what the number means; empty when nothing was found.

## Guidance

Returned by `ito_integral`, `strat_integral` and `marcus_integral`.

## Practical Examples

### Example 1

Reading a result. The integral `∫ W dW` of one path with its quadratic variation and the identity `∫W dW = (W² − QV)/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 1000, 3);

  const auto r = sde::ito_integral([](double, const Series &w) { return w; }, path);
  const double w = path.w.select(0, 1000).sum().item<double>();
  out.writeln("value =", r.value.items<double>(), ", quadratic variation =", r.realized_quadratic_variation.items<double>());
  out.writeln("(W^2 - QV) / 2 =", 0.5 * (w * w - r.realized_quadratic_variation.sum().item<double>()), ", ratio =", r.quadratic_variation_ratio);
}
```

Output

```text
value = [-0.249051] , quadratic variation = [0.987343]
(W^2 - QV) / 2 = -0.249051 , ratio = 0.987343
```

### Example 2

Warnings. A path scaled to volatility 3 has quadratic variation about 9 times the elapsed time; the result says so.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});

  sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 1000, 4);
  path.w = path.w * 3.0;
  const auto r = sde::ito_integral([](double, const Series &w) { return w; }, path);
  out.writeln("ratio =", r.quadratic_variation_ratio);
  for (const std::string &message : r.warnings) out.writeln("warning:", message);
}
```

Output

```text
ratio = 8.401
warning: the path's realized quadratic variation is 8.400991 times the elapsed time; a standard Wiener path gives about 1, so this is not a standard Brownian path (another volatility, a smooth path, or jumps) and Ito and Stratonovich values are not the standard ones
```

### Example 3

A batch. The ratio is a batch mean; the value and the quadratic variation keep the batch shape.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series like = Series::zeros({4}, pluss::ScalarType::Float64);
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 500, 3);

  const auto r = sde::ito_integral([](double, const Series &w) { return w; }, path);
  out.writeln("value shape =", r.value.sizes(), ", ratio =", r.quadratic_variation_ratio, ", warnings =", r.warnings.size());
}
```

Output

```text
value shape = [4] , ratio = 0.979 , warnings = 0
```

## See Also

- [`ito_integral`](ito_integral.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="JumpPath.html">
    <span class="label">Previous</span>
    <span class="title">JumpPath</span>
  </a>
  <a class="next-link" href="WienerPath.html">
    <span class="label">Next</span>
    <span class="title">WienerPath</span>
  </a>
</div>
<!-- apiref:nav:end -->
