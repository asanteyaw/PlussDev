<!-- generated from simulate_wiener_path.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::simulate_wiener_path`

Simulates standard Wiener paths on an equal grid and returns them with their times.

## Signature

```cpp
sde::WienerPath sde::simulate_wiener_path(const pluss::Series &like, double t0, double t1, std::int64_t steps, std::uint64_t seed);
```

## What It Does

A standard Wiener path starts at 0 and has independent Gaussian increments with variance equal to the step: `W(tᵢ₊₁) − W(tᵢ) ~ N(0, h)` with `h = (t1 − t0)/steps`. `like` is a template that fixes the batch shape, data type and device: the result has shape `[steps + 1] + like.shape`, so a `like` of shape `[N]` simulates `N` independent paths at once. Its values are not used. `W(t0)` is defined to be 0.

The path is returned as a value so that you can integrate along it several ways (the Itô and Stratonovich integrals of the same data differ by exactly half the quadratic variation of that very path) and reuse it for cross-checks. The same seed always gives the same path.

## Parameters

`const pluss::Series &like`
: Template for the batch shape, dtype and device; values unused.

`double t0, double t1`
: Start and end times.

`std::int64_t steps`
: Number of equal steps; must be positive.

`std::uint64_t seed`
: Seed of the random stream.

## Returns

A [`WienerPath`](WienerPath.md) of `steps + 1` samples.

## Errors

- `steps <= 0` throws (`pluss::Error`).

## Practical Examples

### Example 1

A path and its grid. 5 steps over `[0, 1]`: the grid has 6 times and the path starts at zero.

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

  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 5, 3);
  out.writeln("times =", path.times);
  out.writeln("W =", path.w.reshape({6}).items<double>());
}
```

Output

```text
times = [0.0000, 0.2000, 0.4000, 0.6000, 0.8000, 1.0000]
W = [0.0000, -0.2867, -1.1746, -0.7777, -0.5821, -1.0950]
```

### Example 2

It is a Wiener path: the endpoint of many paths has mean 0 and variance `t1 − t0`, and the squared increments add up to the elapsed time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = Series::zeros({20000}, pluss::ScalarType::Float64);

  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 2.0, 400, 5);
  const Series end = path.w.select(0, 400);
  out.writeln("mean W(T) =", end.mean().sum().item<double>(), ", variance =", end.square().mean().sum().item<double>(), " (T = 2)");
  out.writeln("mean quadratic variation =", path.w.diff(1, 0).square().sum(0).mean().sum().item<double>(), " (T = 2)");
}
```

Output

```text
mean W(T) = 0.0064 , variance = 2.0220  (T = 2)
mean quadratic variation = 2.0014  (T = 2)
```

### Example 3

Batches. A template of shape `[3, 2]` gives six independent paths; the time axis comes first.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const Series like = Series::zeros({3, 2}, pluss::ScalarType::Float64);
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 10, 1);
  out.writeln("shape =", path.w.sizes());
}
```

Output

```text
shape = [11, 3, 2]
```

### Example 4

Reproducibility. The same seed and arguments give identical paths; another seed gives different ones.

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

  const Series a = sde::simulate_wiener_path(like, 0.0, 1.0, 50, 8).w;
  const Series b = sde::simulate_wiener_path(like, 0.0, 1.0, 50, 8).w;
  const Series c = sde::simulate_wiener_path(like, 0.0, 1.0, 50, 9).w;
  out.writeln("same seed: max difference =", (a - b).abs().max().sum().item<double>(), ", other seed:", (a - c).abs().max().sum().item<double>());
}
```

Output

```text
same seed: max difference = 0.0000 , other seed: 0.8338
```

### Example 5

A non-positive step count is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});

  try {
    (void)sde::simulate_wiener_path(like, 0.0, 1.0, 0, 1);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::simulate_wiener_path: steps must be > 0
```

## See Also

- [`WienerPath`](WienerPath.md): what it returns
- [`ito_integral`](ito_integral.md): integrate along it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="simulate_jump_diffusion_path.html">
    <span class="label">Previous</span>
    <span class="title">simulate_jump_diffusion_path</span>
  </a>
  <a class="next-link" href="strat_integral.html">
    <span class="label">Next</span>
    <span class="title">strat_integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
