<!-- generated from AdaptednessOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::AdaptednessOptions`

How hard the sampled-path integral forms probe an integrand for dependence on the future.

An integrand computed from the whole path is adapted when its value at sample `i` depends only on samples `0 … i`. The check keeps the first `j + 1` samples of the path, redraws the rest with fresh Wiener increments, recomputes the integrand and compares samples `0 … j`; any difference above `tolerance` (relative to the integrand's size) means it depended on the future. The split point `j` runs across the path over the probes. The probe is randomised and finite, so a dependence that happens to cancel for every redraw is not seen: raise `probes` for an integrand you are unsure of.

## Fields

### `probes`

`std::int64_t`, default `4`.

Number of redraws. 0 disables the check. Raise it for an integrand you are unsure of.

### `seed`

`std::uint64_t`, default `12345`.

Seed of the redrawn increments.

### `tolerance`

`double`, default `1e-9`.

Largest change, relative to `1 + max |integrand|`, accepted at an unchanged sample before the integrand is declared anticipating.

## Guidance

Passed as the last argument of the sampled-path forms of `ito_integral` and `strat_integral`.

## Practical Examples

### Example 1

The probe count. A dependence on the next sample shows at every split, so even one probe catches it; the default of four is cheap insurance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, 5);

  auto next_sample = [](const Series &w) {
    const std::int64_t n = w.size(0);
    return w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
  };
  for (const std::int64_t probes : {1, 4}) {
    sde::AdaptednessOptions check;
    check.probes = probes;
    try {
      (void)sde::ito_integral(sde::SampledIntegrandFn{next_sample}, path, check);
      out.writeln("probes", probes, ": accepted");
    } catch (const std::exception &) {
      out.writeln("probes", probes, ": refused");
    }
  }
}
```

Output

```text
probes 1 : refused
probes 4 : refused
```

### Example 2

Switching the check off. With `probes = 0` no check is made and an anticipating integrand is integrated without complaint, which is the silent wrong answer the check exists to prevent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, 5);

  auto next_sample = [](const Series &w) {
    const std::int64_t n = w.size(0);
    return w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
  };
  sde::AdaptednessOptions off;
  off.probes = 0;
  const auto r = sde::ito_integral(sde::SampledIntegrandFn{next_sample}, path, off);
  out.writeln("no check: value =", r.value.items<double>(), "(the adapted Ito integral of W on this path is", 0.5 * (path.w.select(0, 200).sum().item<double>() * path.w.select(0, 200).sum().item<double>() - 1.0), ")");
}
```

Output

```text
no check: value = [0.6332] (the adapted Ito integral of W on this path is -0.3023 )
```

### Example 3

The seed picks the redrawn noise; it changes the probes, not the verdict, for a clear dependence.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, 5);

  auto next_sample = [](const Series &w) {
    const std::int64_t n = w.size(0);
    return w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
  };
  for (const std::uint64_t seed : {1, 99}) {
    sde::AdaptednessOptions check;
    check.seed = seed;
    try {
      (void)sde::ito_integral(sde::SampledIntegrandFn{next_sample}, path, check);
      out.writeln("seed", seed, ": accepted");
    } catch (const std::exception &) {
      out.writeln("seed", seed, ": refused");
    }
  }
}
```

Output

```text
seed 1 : refused
seed 99 : refused
```

### Example 4

The tolerance. An integrand that depends on the future only through a very small term passes a loose tolerance and fails a tight one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, 5);

  auto nearly_adapted = [](const Series &w) {
    const std::int64_t n = w.size(0);
    return w + w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0) * 1e-6;
  };
  for (const double tolerance : {1e-3, 1e-9}) {
    sde::AdaptednessOptions check;
    check.tolerance = tolerance;
    try {
      (void)sde::ito_integral(sde::SampledIntegrandFn{nearly_adapted}, path, check);
      out.writeln("tolerance", tolerance, ": accepted");
    } catch (const std::exception &) {
      out.writeln("tolerance", tolerance, ": refused");
    }
  }
}
```

Output

```text
tolerance 0.001000000 : accepted
tolerance 0.000000001 : refused
```

## See Also

- [`ito_integral`](ito_integral.md): uses it
- [`strat_integral`](strat_integral.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="JumpPath.html">
    <span class="label">Next</span>
    <span class="title">JumpPath</span>
  </a>
</div>
<!-- apiref:nav:end -->
