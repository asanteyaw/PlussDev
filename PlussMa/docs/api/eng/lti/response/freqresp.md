<!-- generated from freqresp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::freqresp`

Evaluates a system's frequency response on a grid.

## Signature

```cpp
pluss::Series lti::freqresp(
    const lti::StateSpace& system,
    const pluss::Series& omega);

pluss::Series lti::freqresp(
    const lti::PoleZero& system,
    const pluss::Series& omega);
```

## What It Does

Returns `G(j w)` for a continuous system or `G(e^{j w T})` for a discrete one with sample time `T`, for each angular frequency `w` in rad/s. There is an overload for a `StateSpace` (`C (j w I - A)^{-1} B + D`) and one for a `PoleZero`.

## Parameters

`system`
: A `StateSpace` or `PoleZero`.

`omega`
: A non-empty rank-one floating `Series` of angular frequencies, rad/s.

## Returns

For a `StateSpace`, a complex `Series` of shape `[outputs, inputs, omega]` (with batch axes); for a `PoleZero`, a complex `Series` with the shape of `omega`.

## Errors

A scalar, empty or integer `omega` throws `pluss::Error`.

## Interpretation and Use

The first example compares `1 / (s + 1)` with `1 / (1 + j w)` at three frequencies. The third shows that the state-space and pole-zero forms of the same system agree to rounding. Take `abs()` for the gain and `arg()` for the phase; the gain in decibels is `20 log10 |G|`.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A first-order system against its closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series omega = pluss::make_series({0.0, 1.0, 10.0});
  const auto h = lti::freqresp(s, omega).flatten().items<std::complex<double>>();
  for (std::size_t i = 0; i < h.size(); ++i) {
    const double w = i == 0 ? 0.0 : (i == 1 ? 1.0 : 10.0);
    const std::complex<double> exact = 1.0 / std::complex<double>(1.0, w);
    out.writeln("omega", w, ": H =", h[i], " exact 1 / (1 + j omega) =", exact);
  }
}
```

Output

```text
omega 0.0000 : H = (1,0)  exact 1 / (1 + j omega) = (1,0)
omega 1.0000 : H = (0.5,-0.5)  exact 1 / (1 + j omega) = (0.5,-0.5)
omega 10.0000 : H = (0.00990099,-0.0990099)  exact 1 / (1 + j omega) = (0.00990099,-0.0990099)
```
### Example 2

The shape for one and for several inputs and outputs.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0});
  const pluss::Series siso = lti::freqresp(oscillator(), omega);
  out.writeln("one input, one output, three frequencies: shape", siso.size(0), "x", siso.size(1), "x", siso.size(2));
  const lti::StateSpace mimo = lti::state(pluss::series::eye(3, pluss::ScalarType::Float64).mul(-1.0), pluss::series::ones({3, 2}, pluss::ScalarType::Float64), pluss::series::ones({2, 3}, pluss::ScalarType::Float64), pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  const pluss::Series m = lti::freqresp(mimo, omega);
  out.writeln("two inputs, two outputs, three frequencies: shape", m.size(0), "x", m.size(1), "x", m.size(2));
}
```

Output

```text
one input, one output, three frequencies: shape 1 x 1 x 3
two inputs, two outputs, three frequencies: shape 2 x 2 x 3
```
### Example 3

One system in both representations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // G(s) = 1 / (s + 1) as a state-space model and as a pole-zero model.
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const lti::PoleZero z = lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({-1.0}), pluss::make_series(1.0));
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 5.0});
  const auto a = lti::freqresp(s, omega).flatten().items<std::complex<double>>();
  const auto b = lti::freqresp(z, omega).items<std::complex<double>>();
  double worst = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) worst = std::max(worst, std::abs(a[i] - b[i]));
  out.writeln("largest difference between the two representations =", worst);
}
```

Output

```text
largest difference between the two representations = 0.0000
```
### Example 4

Every refused grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  refused(out, "a scalar frequency grid", [] { lti::freqresp(oscillator(), pluss::make_series(1.0)); });
  refused(out, "an integer frequency grid", [] { lti::freqresp(oscillator(), pluss::make_series({1, 2})); });
  refused(out, "an empty frequency grid", [] { lti::freqresp(oscillator(), pluss::series::zeros({0}, pluss::ScalarType::Float64)); });
}
```

Output

```text
a scalar frequency grid : eng::lti::freqresp: omega must be a non-empty vector
an integer frequency grid : eng::lti::freqresp: omega must have a floating dtype
an empty frequency grid : eng::lti::freqresp: omega must be a non-empty vector
```
## See Also

See [Pole-zero and response](../response.md) for timebase, stability, and angular-frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_sos.html">
    <span class="label">Previous</span>
    <span class="title">to_sos</span>
  </a>
</div>
<!-- apiref:nav:end -->
