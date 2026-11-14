<!-- generated from ito_integral.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `sde::ito_integral`

The Itô integral `∫ f dW` of an integrand along an explicit path, with the integrand read at the start of each step.

## Signature

```cpp
sde::StochasticIntegralResult sde::ito_integral(const sde::WienerIntegrandFn &f, const sde::WienerPath &path);
sde::StochasticIntegralResult sde::ito_integral(const sde::SampledIntegrandFn &make, const sde::WienerPath &path, const sde::AdaptednessOptions &check = {});
```

## What It Does

The Itô integral is the limit of the left-point sums `Σ f(tᵢ, W(tᵢ)) (W(tᵢ₊₁) − W(tᵢ))`. Reading the integrand at the **start** of the step is not a numerical choice: it is what makes the integrand *non-anticipating* (it cannot know the increment it is about to multiply) and the integral a martingale. Any other reading, such as the average of both ends, gives a different object, the Stratonovich integral ([`strat_integral`](strat_integral.md)), which differs by a fixed amount no refinement removes. For `f = W`: `∫ W dW = (W(T)² − T)/2`.

There are two ways to say what the integrand is.

- **Pointwise** (first signature): `f(t, w)` is a function of the time and the current value of the path, one step at a time. It cannot see the future by construction.
- **From the sampled path** (second signature): `make(w)` receives the whole path and returns the integrand at every sample, which allows running maxima, moving averages or filters. Such an integrand can accidentally use a *later* sample (a centred moving average does), which silently turns the Itô sum into something else. The function therefore **probes** it first: it keeps the first `j + 1` samples of the path, redraws the rest, and requires the integrand at samples `0 … j` to be unchanged; otherwise it throws, naming the first offending sample. The probe is randomised and finite (see [`AdaptednessOptions`](AdaptednessOptions.md)).

Both forms validate the path and report the realised quadratic variation.

## Parameters

`const sde::WienerIntegrandFn &f`
: `Series (double t, const Series &w)`, read at the start of each step; shaped like the batch.

`const sde::SampledIntegrandFn &make`
: `Series (const Series &path)`: the integrand at every sample; checked for adaptedness.

`const sde::WienerPath &path`
: The path to integrate along.

`const sde::AdaptednessOptions &check`
: Probes, seed and tolerance of the adaptedness check.

## Returns

A [`StochasticIntegralResult`](StochasticIntegralResult.md).

## Errors

- A missing integrand, an undefined or inconsistent path, a non-finite integrand, or an integrand that depends on later samples throws (`pluss::Error`).

## Practical Examples

### Example 1

The defining example. For any path, the left-point sum satisfies the exact identity `Σ W(tᵢ) ΔWᵢ = (W(T)² − Σ ΔWᵢ²)/2`; the realised quadratic variation tends to `T`, which gives `∫₀ᵀ W dW = (W(T)² − T)/2` in the limit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});

  for (const std::int64_t steps : {50, 500, 5000}) {
    const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, 6);
    const auto r = sde::ito_integral([](double, const Series &w) { return w; }, path);
    const double wT = path.w.select(0, steps).sum().item<double>();
    const double qv = r.realized_quadratic_variation.sum().item<double>();
    out.writeln("steps", steps, ": integral =", r.value.sum().item<double>(), ", (W^2 - QV)/2 =", 0.5 * (wT * wT - qv), ", QV =", qv);
  }
}
```

Output

```text
steps 50 : integral = -0.45401 , (W^2 - QV)/2 = -0.45401 , QV = 0.91699
steps 500 : integral = -0.43280 , (W^2 - QV)/2 = -0.43280 , QV = 0.97209
steps 5000 : integral = -0.09666 , (W^2 - QV)/2 = -0.09666 , QV = 1.03850
```

### Example 2

A deterministic integrand. `∫₀¹ t dW` is Gaussian with mean 0 and variance `∫ t² dt = 1/3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = Series::zeros({40000}, pluss::ScalarType::Float64);

  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, 2);
  const auto r = sde::ito_integral([](double t, const Series &w) { return w * 0.0 + t; }, path);
  out.writeln("mean =", r.value.mean().sum().item<double>(), ", variance =", r.value.square().mean().sum().item<double>(), ", exact variance =", 1.0 / 3.0);
}
```

Output

```text
mean = 0.0009 , variance = 0.3356 , exact variance = 0.3333
```

### Example 3

The martingale property: `E[∫ f dW] = 0` even for a path-dependent integrand like `W`, which the Stratonovich integral does not share.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Series like = Series::zeros({40000}, pluss::ScalarType::Float64);

  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 200, 9);
  const auto ito = sde::ito_integral([](double, const Series &w) { return w; }, path);
  const auto strat = sde::strat_integral([](double, const Series &w) { return w; }, path);
  out.writeln("mean of the Ito integral =", ito.value.mean().sum().item<double>(), ", of the Stratonovich integral =", strat.value.mean().sum().item<double>(), " (T/2 = 0.5)");
}
```

Output

```text
mean of the Ito integral = 0.0046 , of the Stratonovich integral = 0.5048  (T/2 = 0.5)
```

### Example 4

From the sampled path: a running maximum is adapted, so it passes the check and is integrated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace sde = pluss::subsp::math::sde;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const Series like = pluss::make_series(0.0).to(pluss::ScalarType::Float64).reshape({1});
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 300, 9);

  auto running_max = [](const Series &w) {
    Series best = w.select(0, 0);
    Series result = best.unsqueeze(0);
    for (std::int64_t i = 1; i < w.size(0); ++i) {
      best = best.maximum(w.select(0, i));
      result = result.unite({best.unsqueeze(0)}, 0);
    }
    return result;
  };
  const auto r = sde::ito_integral(sde::SampledIntegrandFn{running_max}, path);
  out.writeln("integral of the running maximum =", r.value.items<double>(), ", warnings =", r.warnings.size());
}
```

Output

```text
integral of the running maximum = [-0.93973] , warnings = 0
```

### Example 5

From the sampled path: a centred moving average uses the next sample, so it is not adapted and the call refuses it, naming the sample where the dependence shows.

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
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 300, 9);

  auto centred = [](const Series &w) {
    const std::int64_t n = w.size(0);
    const Series previous = w.slice(0, 0, 1, 1).unite({w.slice(0, 0, n - 1, 1)}, 0);
    const Series next = w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
    return (previous + w + next) / 3.0;
  };
  try {
    (void)sde::ito_integral(sde::SampledIntegrandFn{centred}, path);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::ito_integral: the integrand at sample 60 changed when only samples after sample 60 of the path were redrawn, so it looks into the future of the path (it is not adapted) and the sum is not an Ito integral
```

### Example 6

A missing integrand and a malformed path are rejected.

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
  const sde::WienerPath path = sde::simulate_wiener_path(like, 0.0, 1.0, 10, 3);

  try {
    (void)sde::ito_integral(sde::WienerIntegrandFn{}, path);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::sde::ito_integral: integrand callback is required
```

## See Also

- [`StochasticIntegralResult`](StochasticIntegralResult.md): what it returns
- [`WienerPath`](WienerPath.md): the path
- [`strat_integral`](strat_integral.md): the Stratonovich convention
- [`AdaptednessOptions`](AdaptednessOptions.md): the look-ahead check

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="WienerPath.html">
    <span class="label">Previous</span>
    <span class="title">WienerPath</span>
  </a>
  <a class="next-link" href="marcus_integral.html">
    <span class="label">Next</span>
    <span class="title">marcus_integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
