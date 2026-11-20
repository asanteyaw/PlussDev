<!-- generated from pid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::pid`

Builds a PID controller with a filtered derivative.

## Signature

```cpp
control::Pid control::pid(
    const pluss::Series& kp,
    const pluss::Series& ki,
    const pluss::Series& kd,
    const pluss::Series& filter,
    double sample_time = 0.0,
    control::PidRule int_rule = control::PidRule::Forward,
    control::PidRule diff_rule = control::PidRule::Forward);
```

## What It Does

Creates a `Pid` whose continuous transfer function is `C(s) = Kp + Ki / s + Kd s / (T_f s + 1)`, where `T_f` is the derivative-filter time constant. A positive `sample_time` makes it a discrete controller: the integral and the filtered derivative are replaced by the substitutions chosen with `PidRule`. The gains may carry leading batch axes, one controller per lane.

## Parameters

`kp`, `ki`, `kd`
: Proportional, integral and derivative gains: real floating `Series`, scalar or batched.

`filter`
: The derivative-filter time constant `T_f`, non-negative. Zero leaves the derivative unfiltered.

`sample_time`
: Zero (the default) for a continuous controller, otherwise the sample period in seconds.

`int_rule`, `diff_rule`
: The discrete substitutions for the integrator and the derivative (`PidRule`); ignored when continuous.

## Returns

A `Pid`. Gains of different dtypes promote to a common floating dtype.

## Errors

A negative filter, a negative or non-finite sample time, an integer gain, gains on different platforms or of incompatible batch shapes, an unfiltered derivative with the `Trapezoid` rule, and a `Forward` derivative with `sample_time >= 2 * filter` throw `pluss::Error`; the last two would give an unstable discrete derivative.

## Interpretation and Use

The first example evaluates a controller with `Kp = 2`, `Ki = 1`, `Kd = 0.5`, `T_f = 0.1` at three frequencies and finds no difference from the closed form. The second shows the same controller discretised with backward-Euler rules at 100 Hz: it follows the continuous one closely at low frequency and departs from it as `omega T` grows. The third builds three controllers at once from a vector of gains.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A continuous controller against its closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // C(s) = kp + ki / s + kd s / (filter s + 1) with kp = 2, ki = 1, kd = 0.5 and filter = 0.1.
  const ctl::Pid c = ctl::pid(pluss::make_series(2.0), pluss::make_series(1.0), pluss::make_series(0.5), pluss::make_series(0.1));
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0});
  const auto h = ctl::freqresp(c, omega).items<std::complex<double>>();
  double worst = 0.0;
  for (std::size_t i = 0; i < h.size(); ++i) {
    const double w = omega.items<double>()[i];
    const std::complex<double> s(0.0, w);
    const std::complex<double> exact = 2.0 + 1.0 / s + 0.5 * s / (0.1 * s + 1.0);
    worst = std::max(worst, std::abs(h[i] - exact));
    out.writeln("omega", w, ": |C| =", std::abs(h[i]), " phase (rad) =", std::arg(h[i]));
  }
  out.writeln("largest difference from the closed form =", worst);
}
```

Output

```text
omega 0.1000 : |C| = 10.1491  phase (rad) = -1.3724
omega 1.0000 : |C| = 2.1108  phase (rad) = -0.2416
omega 10.0000 : |C| = 5.1000  phase (rad) = 0.4900
largest difference from the closed form = 0.0000
```
### Example 2

The same controller discretised at 100 Hz against the continuous one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Backward-Euler integrator and derivative at 100 Hz against the continuous controller.
  const auto scalar = [](double v) { return pluss::make_series(v); };
  const ctl::Pid continuous = ctl::pid(scalar(2.0), scalar(1.0), scalar(0.5), scalar(0.1));
  const ctl::Pid discrete = ctl::pid(scalar(2.0), scalar(1.0), scalar(0.5), scalar(0.1), 0.01, ctl::PidRule::Backward, ctl::PidRule::Backward);
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0, 50.0});
  const auto a = ctl::freqresp(continuous, omega).items<std::complex<double>>();
  const auto b = ctl::freqresp(discrete, omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < a.size(); ++i) out.writeln("omega", omega.items<double>()[i], ": |C| continuous", std::abs(a[i]), " discrete", std::abs(b[i]));
  out.writeln("sample time", discrete.sample_time(), " discrete:", discrete.is_disc());
}
```

Output

```text
omega 0.1000 : |C| continuous 10.1491  discrete 10.1501
omega 1.0000 : |C| continuous 2.1108  discrete 2.1181
omega 10.0000 : |C| continuous 5.1000  discrete 5.0474
omega 50.0000 : |C| continuous 6.8725  discrete 6.6599
sample time 0.0100  discrete: true
```
### Example 3

Three controllers from a vector of proportional gains.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // Three controllers that differ in their proportional gain, evaluated at two frequencies.
  const ctl::Pid c = ctl::pid(pluss::make_series({1.0, 2.0, 4.0}), pluss::make_series(0.0), pluss::make_series(0.0), pluss::make_series(0.0));
  const pluss::Series h = ctl::freqresp(c, pluss::make_series({1.0, 10.0}));
  out.writeln("response shape", h.size(0), "x", h.size(1), " (controllers by frequencies)");
  out.writeln("proportional-only gains =", h.real().select(1, 0).items<double>());
}
```

Output

```text
response shape 3 x 2  (controllers by frequencies)
proportional-only gains = [1.0000, 2.0000, 4.0000]
```
### Example 4

Gains promoted to one dtype.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ctl::Pid c = ctl::pid(pluss::make_series(2.0f), pluss::make_series(1.0), pluss::make_series(0.5), pluss::make_series(0.1));
  out.writeln("gains promote to a common dtype:", c.kp().scalar_type(), c.ki().scalar_type(), c.kd().scalar_type(), c.filter().scalar_type());
}
```

Output

```text
gains promote to a common dtype: float64 float64 float64 float64
```
### Example 5

Every refused construction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
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
static double max_abs(const pluss::Series &s) { return s.abs().max().item<double>(); }

// A double integrator x'' = u as the matrices of x' = A x + B u.
static pluss::Series A2() { return mat({0.0, 1.0, 0.0, 0.0}, 2, 2); }
static pluss::Series B2() { return mat({0.0, 1.0}, 2, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto s = [](double v) { return pluss::make_series(v); };
  refused(out, "a negative derivative filter", [&] { ctl::pid(s(1.0), s(0.0), s(0.5), s(-0.1)); });
  refused(out, "a negative sample time", [&] { ctl::pid(s(1.0), s(0.0), s(0.0), s(0.0), -0.01); });
  refused(out, "an unfiltered trapezoidal derivative", [&] { ctl::pid(s(1.0), s(0.0), s(0.5), s(0.0), 0.01, ctl::PidRule::Forward, ctl::PidRule::Trapezoid); });
  refused(out, "a forward derivative with sample_time >= 2 filter", [&] { ctl::pid(s(1.0), s(0.0), s(0.5), s(0.004), 0.01, ctl::PidRule::Forward, ctl::PidRule::Forward); });
  refused(out, "an integer gain", [&] { ctl::pid(pluss::make_series(1), s(0.0), s(0.0), s(0.0)); });
}
```

Output

```text
a negative derivative filter : eng::control::pid: filter must be non-negative
a negative sample time : eng::control::pid: sample_time must be finite and non-negative
an unfiltered trapezoidal derivative : eng::control::pid: an unfiltered trapezoidal derivative is unstable
a forward derivative with sample_time >= 2 filter : eng::control::pid: forward derivative requires sample_time < 2*filter
an integer gain : eng::control::pid: Kp must have a real floating dtype
```