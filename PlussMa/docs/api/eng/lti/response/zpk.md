<!-- generated from zpk.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::zpk`

Builds a pole-zero-gain model.

## Signature

```cpp
lti::PoleZero lti::zpk(
    const pluss::Series& zeros,
    const pluss::Series& poles,
    const pluss::Series& gain,
    double sample_time = 0.0);
```

## What It Does

Bundles the zeros, poles and gain of `G = k (s - z_1)...(s - z_m) / ((s - p_1)...(s - p_n))` (or the same in `z` for a discrete system) into a `PoleZero`. A system is proper only if it has no more zeros than poles.

## Parameters

`zeros`
: A rank-one `Series` of zeros; an empty series for none. Floating or complex.

`poles`
: A rank-one `Series` of poles.

`gain`
: A scalar `Series`, the gain `k`.

`sample_time`
: Zero (the default) for a continuous system, positive for a discrete one.

## Returns

A `PoleZero`.

## Errors

A non-finite pole or zero, more zeros than poles, an integer dtype and a negative or non-finite sample time throw `pluss::Error`.

## Interpretation and Use

The first example builds `2 / (s + 2)` and checks its gain at zero frequency (1) and at the corner frequency (`1/sqrt(2)`). Complex poles come in conjugate pairs, as the second example shows for a damped oscillator.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A first-order lag: accessors and gain at DC and at the corner.

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
  // G(s) = 2 / (s + 2): no zeros, one pole at -2 and gain 2.
  const lti::PoleZero g = lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({-2.0}), pluss::make_series(2.0));
  out.writeln("zeros", g.zeros().numel(), " poles", g.poles().items<double>(), " gain", g.gain().real().item<double>());
  const pluss::Series omega = pluss::make_series({0.0, 2.0});
  out.writeln("|G| at 0 and 2 rad/s =", lti::freqresp(g, omega).abs().items<double>(), " expected 1 and 1/sqrt(2) =", 1.0, 1.0 / std::sqrt(2.0));
}
```

Output

```text
zeros 0  poles [-2.0000]  gain 2.0000
|G| at 0 and 2 rad/s = [1.0000, 0.7071]  expected 1 and 1/sqrt(2) = 1.0000 0.7071
```
### Example 2

A conjugate pole pair and a zero.

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
  // Two poles at -0.2 +- 1.99j (the damped oscillator of the other pages) and a zero at -1.
  const std::complex<double> p(-0.2, std::sqrt(4.0 - 0.04));
  const lti::PoleZero g = lti::zpk(pluss::make_series({-1.0}), pluss::make_series({p, std::conj(p)}), pluss::make_series(4.0));
  const auto poles = g.poles().items<std::complex<double>>();
  out.writeln("poles", poles[0], poles[1], " stable:", g.is_stable());
  out.writeln("DC gain =", lti::freqresp(g, pluss::make_series({0.0})).items<std::complex<double>>()[0], " expected 4 / |p|^2 =", 4.0 / std::norm(p));
}
```

Output

```text
poles (-0.2,1.98997) (-0.2,-1.98997)  stable: true
DC gain = (1,0)  expected 4 / |p|^2 = 1.0000
```
### Example 3

A discrete system, stable and unstable.

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
  // z / (z - 0.5) sampled every 0.1 s.
  const lti::PoleZero g = lti::zpk(pluss::make_series({0.0}), pluss::make_series({0.5}), pluss::make_series(1.0), 0.1);
  out.writeln("discrete:", g.is_disc(), " sample time", g.sample_time(), " stable:", g.is_stable());
  const lti::PoleZero outside = lti::zpk(pluss::make_series({0.0}), pluss::make_series({1.2}), pluss::make_series(1.0), 0.1);
  out.writeln("pole at 1.2: stable:", outside.is_stable());
}
```

Output

```text
discrete: true  sample time 0.1000  stable: true
pole at 1.2: stable: false
```
### Example 4

Every refused input.

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
  const pluss::Series none = pluss::series::empty({0}, pluss::ScalarType::Float64);
  refused(out, "a non-finite pole", [&] { lti::zpk(none, pluss::make_series({std::nan("")}), pluss::make_series(1.0)); });
  refused(out, "more zeros than poles", [&] { lti::zpk(pluss::make_series({1.0, 2.0}), pluss::make_series({-1.0}), pluss::make_series(1.0)); });
  refused(out, "an integer pole array", [&] { lti::zpk(none, pluss::make_series({-1}), pluss::make_series(1.0)); });
  refused(out, "a negative sample time", [&] { lti::zpk(none, pluss::make_series({-1.0}), pluss::make_series(1.0), -0.1); });
}
```

Output

```text
a non-finite pole : eng::lti::zpk: poles must be finite
more zeros than poles : eng::lti::zpk: a proper system cannot have more zeros than poles
an integer pole array : eng::lti::zpk: poles must have a floating or complex dtype
a negative sample time : eng::lti::zpk: sample_time must be finite and non-negative
```
## See Also

See [Pole-zero and response](../response.md) for timebase, stability, and angular-frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PoleZero.html">
    <span class="label">Previous</span>
    <span class="title">PoleZero</span>
  </a>
  <a class="next-link" href="to_sos.html">
    <span class="label">Next</span>
    <span class="title">to_sos</span>
  </a>
</div>
<!-- apiref:nav:end -->
