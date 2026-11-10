<!-- generated from AleatoriaResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::AleatoriaResult`

A Monte Carlo estimate with its standard error.

`value` is the estimate (a scalar `Series`, complex for a complex integrand), `standard_error` the standard deviation of the estimate (the sample standard deviation of the weighted values over `√samples`), and `samples` the number of points. About two thirds of the time the exact value lies within one standard error of `value`, and about 95% of the time within two.

## Fields

### `value`

`pluss::Series`, default `undefined`.

The estimate.

### `standard_error`

`double`, default `0`.

Standard deviation of the estimate.

### `samples`

`std::int64_t`, default `0`.

Points used.

## Guidance

Returned by `aleatoria`.

## Practical Examples

### Example 1

Reading a result. The exact `∫₀¹ x² dx = 1/3` is within a standard error or two of the estimate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  integ::AleatoriaOptions options;
  options.samples = 100000;
  options.seed = 9;
  const integ::AleatoriaResult r = integ::aleatoria([](const Series &t) { return t.square(); }, 0.0, 1.0, options);
  out.writeln("estimate =", r.value.item<double>(), ", standard error =", r.standard_error, ", samples =", r.samples);
  out.writeln("distance in standard errors =", std::abs(r.value.item<double>() - 1.0 / 3.0) / r.standard_error);
}
```

Output

```text
estimate = 0.333478 , standard error = 0.000944 , samples = 100000
distance in standard errors = 0.153506
```

### Example 2

A complex integrand. `∫₀¹ e^{ix} dx = sin 1 + i (1 − cos 1) = 0.8415 + 0.4597i`. The integrand is complex, so the estimate and its standard error are too.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  integ::AleatoriaOptions options;
  options.samples = 200000;
  options.seed = 6;
  options.stratified = true;
  const Series i = pluss::make_series(std::vector<std::complex<double>>{{0.0, 1.0}}, {1}).to(pluss::ScalarType::Complex128);   // the imaginary unit
  const integ::AleatoriaResult r = integ::aleatoria([&](const Series &t) { return (t.to(pluss::ScalarType::Complex128) * i).exp(); }, 0.0, 1.0, options);
  const std::complex<double> v = r.value.item<std::complex<double>>();
  out.writeln("estimate =", v.real(), v.imag(), ", exact =", std::sin(1.0), 1.0 - std::cos(1.0));
}
```

Output

```text
estimate = 0.84147 0.45970 , exact = 0.84147 0.45970
```

### Example 3

A noisy integrand. A discontinuous function has no good quadrature rule, but Monte Carlo is unaffected: the indicator of `x < 0.3` integrates to `0.3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  integ::AleatoriaOptions options;
  options.samples = 200000;
  options.seed = 8;
  const integ::AleatoriaResult r = integ::aleatoria([](const Series &t) { return (t < 0.3).to(pluss::ScalarType::Float64); }, 0.0, 1.0, options);
  out.writeln("estimate =", r.value.item<double>(), ", standard error =", r.standard_error);
}
```

Output

```text
estimate = 0.29768 , standard error = 0.00102
```

## See Also

- [`aleatoria`](aleatoria.md): produces it
- [`AleatoriaOptions`](AleatoriaOptions.md): the settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="AleatoriaOptions.html">
    <span class="label">Previous</span>
    <span class="title">AleatoriaOptions</span>
  </a>
  <a class="next-link" href="IntegraOptions.html">
    <span class="label">Next</span>
    <span class="title">IntegraOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
