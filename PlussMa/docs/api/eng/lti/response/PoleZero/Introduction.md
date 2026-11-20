<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::PoleZero` Introduction

Holds the zeros, poles and gain of a transfer function and its sample time. It is returned by `zpk` and by the filter designs (`butter`, `cheby1`, `cheby2`), and consumed by `freqresp` and `to_sos`.

`is_stable` is strict: a pole at the origin or on the imaginary axis (continuous), or on the unit circle (discrete), is not stable, as the second example shows. A fourth-order Butterworth design has all four poles inside the unit circle (largest magnitude about 0.80).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The three components and the time base.

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
  const lti::PoleZero g = lti::zpk(pluss::make_series({-1.0}), pluss::make_series({-2.0, -3.0}), pluss::make_series(5.0));
  out.writeln("zeros =", g.zeros().items<double>(), " poles =", g.poles().items<double>(), " gain =", g.gain().real().item<double>());
  out.writeln("continuous:", g.is_cont(), " discrete:", g.is_disc(), " sample time", g.sample_time());
}
```

Output

```text
zeros = [-1.0000]  poles = [-2.0000, -3.0000]  gain = 5.0000
continuous: true  discrete: false  sample time 0.0000
```
### Example 2

`is_stable` for continuous and discrete poles on and off the boundary.

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
  const auto make = [](double pole, double sample_time) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({pole}), pluss::make_series(1.0), sample_time); };
  for (const double p : {-1.0, 0.0, 1.0}) out.writeln("continuous pole", p, ": stable", make(p, 0.0).is_stable());
  for (const double p : {0.5, 1.0, 1.5}) out.writeln("discrete pole", p, ": stable", make(p, 0.1).is_stable());
}
```

Output

```text
continuous pole -1.0000 : stable true
continuous pole 0.0000 : stable false
continuous pole 1.0000 : stable false
discrete pole 0.5000 : stable true
discrete pole 1.0000 : stable false
discrete pole 1.5000 : stable false
```
### Example 3

A filter design as a `PoleZero`.

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
  // The filter designs return a PoleZero: the poles of a fourth-order Butterworth low-pass sit on the unit circle's interior.
  const lti::PoleZero design = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const auto poles = design.poles().items<std::complex<double>>();
  double largest = 0.0;
  for (const auto &p : poles) largest = std::max(largest, std::abs(p));
  out.writeln("poles", poles.size(), " zeros", design.zeros().numel(), " discrete:", design.is_disc(), " sample time", design.sample_time());
  out.writeln("largest pole magnitude =", largest, " stable:", design.is_stable());
}
```

Output

```text
poles 4  zeros 4  discrete: true  sample time 0.0010
largest pole magnitude = 0.7954  stable: true
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="gain.html">
    <span class="label">Next</span>
    <span class="title">gain</span>
  </a>
</div>
<!-- apiref:nav:end -->
