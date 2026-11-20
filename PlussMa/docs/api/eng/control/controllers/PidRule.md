<!-- generated from PidRule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::PidRule`

How a discrete PID replaces the integral and the derivative.

## Signature

```cpp
control::PidRule
```

## What It Does

A continuous integrator `1/s` and filtered derivative can be replaced by a difference equation in several ways. `Forward` uses the rectangle at the earlier sample, `Backward` the rectangle at the later sample and `Trapezoid` the average of the two. The rule decides the accuracy of the frequency response and which derivative settings are stable.

## Values

| Value | Integrator `1/s` becomes | Behaviour |
|---|---|---|
| `Forward` | `T / (z - 1)` | Magnitude slightly high; the phase lags by `omega T / 2`. |
| `Backward` | `T z / (z - 1)` | The same magnitude as `Forward`; the phase leads by `omega T / 2`. |
| `Trapezoid` | `(T / 2)(z + 1) / (z - 1)` | Exactly the ideal phase; magnitude low at high frequency. |

## Returns

Arguments `int_rule` and `diff_rule` of `pid`.

## Errors

Not every rule is admissible for the derivative: an unfiltered `Trapezoid` derivative and a `Forward` derivative with `sample_time >= 2 * filter` are rejected at construction.

## Interpretation and Use

The first example prints the response of a discrete integrator at three frequencies: `Forward` and `Backward` have equal magnitude and phase errors of equal size and opposite sign (at `omega = 10` and `T = 0.05` the phases are -1.82 and -1.32 against the ideal -1.57), while `Trapezoid` keeps the phase exactly and loses magnitude. Use `Trapezoid` when phase matters, `Backward` for the integrator when the delay of `Forward` must be avoided.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The integrator under each rule at three frequencies.

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
  for (const double w : {1.0, 10.0, 40.0}) {
    const pluss::Series one = pluss::make_series({w});
    out.writeln("omega", w, ": ideal magnitude", 1.0 / w, " phase", -std::acos(-1.0) / 2.0);
    for (const auto [label, rule] : {std::pair{"Forward  ", ctl::PidRule::Forward}, {"Backward ", ctl::PidRule::Backward}, {"Trapezoid", ctl::PidRule::Trapezoid}}) {
      const std::complex<double> g = ctl::freqresp(ctl::pid(s(0.0), s(1.0), s(0.0), s(0.0), 0.05, rule, ctl::PidRule::Backward), one).items<std::complex<double>>()[0];
      out.writeln("   ", label, ": magnitude", std::abs(g), " phase", std::arg(g));
    }
  }
}
```

Output

```text
omega 1.0000 : ideal magnitude 1.0000  phase -1.5708
    Forward   : magnitude 1.0001  phase -1.5958
    Backward  : magnitude 1.0001  phase -1.5458
    Trapezoid : magnitude 0.9998  phase -1.5708
omega 10.0000 : ideal magnitude 0.1000  phase -1.5708
    Forward   : magnitude 0.1010  phase -1.8208
    Backward  : magnitude 0.1010  phase -1.3208
    Trapezoid : magnitude 0.0979  phase -1.5708
omega 40.0000 : ideal magnitude 0.0250  phase -1.5708
    Forward   : magnitude 0.0297  phase -2.5708
    Backward  : magnitude 0.0297  phase -0.5708
    Trapezoid : magnitude 0.0161  phase -1.5708
```
### Example 2

Which derivative settings each rule accepts.

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
  // The rule also decides which derivative settings are admissible at a given sample time.
  for (const auto [label, rule] : {std::pair{"Forward  ", ctl::PidRule::Forward}, {"Backward ", ctl::PidRule::Backward}, {"Trapezoid", ctl::PidRule::Trapezoid}}) {
    refused(out, label, [&] { ctl::pid(s(1.0), s(0.0), s(0.5), s(0.004), 0.01, ctl::PidRule::Backward, rule); });
  }
}
```

Output

```text
Forward   : eng::control::pid: forward derivative requires sample_time < 2*filter
Backward  : accepted
Trapezoid : accepted
```
### Example 3

The unfiltered derivative under each rule.

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
  for (const auto [label, rule] : {std::pair{"Forward  ", ctl::PidRule::Forward}, {"Backward ", ctl::PidRule::Backward}, {"Trapezoid", ctl::PidRule::Trapezoid}}) {
    refused(out, label, [&] { ctl::pid(s(1.0), s(0.0), s(0.5), s(0.0), 0.01, ctl::PidRule::Backward, rule); });
  }
}
```

Output

```text
Forward   : accepted
Backward  : accepted
Trapezoid : eng::control::pid: an unfiltered trapezoidal derivative is unstable
```