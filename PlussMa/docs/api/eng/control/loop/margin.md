<!-- generated from margin.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::margin`

Computes the gain, phase and stability margins of a single-input loop.

## Signature

```cpp
control::MarginResult control::margin(
    const pluss::Series& response,
    const pluss::Series& omega);

control::MarginResult control::margin(
    const lti::StateSpace& system,
    const pluss::Series& omega);

control::MarginResult control::margin(
    const lti::PoleZero& system,
    const pluss::Series& omega);
```

## What It Does

Finds on a frequency grid where the Nyquist curve of `L` crosses the unit circle and the negative real axis. The gain margin is the factor by which the loop gain can grow before the closed loop is unstable (linear, not decibels); the phase margin is the extra phase lag, in degrees, that the loop tolerates at the gain crossover; the stability margin is the smallest distance `|1 + L|` from the Nyquist curve to the critical point `-1`. Each margin comes with the frequency where it occurs. The loop may be given as a precomputed response, a `StateSpace` or a `PoleZero`.

## Parameters

`response`
: A complex `Series` of loop responses with the frequency on its last axis; or `system` (a single-input, single-output `StateSpace` or `PoleZero`) to have the response computed.

`omega`
: A strictly increasing real rank-one `Series` of at least two frequencies, rad/s.

## Returns

A `MarginResult`. For a batch of responses each field has the batch shape.

## Errors

A grid with fewer than two points or that does not increase, a response whose frequency axis does not match `omega`, and a system with more than one input or output throw `pluss::Error`.

## Interpretation and Use

For `L = 1 / (s (s + 1) (s + 2))` the phase reaches -180 degrees at `sqrt(2)`, where `|L| = 1/6`, so the gain margin is 6 at 1.4142 rad/s; the first example finds 6.0001, the difference being the resolution of the grid. The gain margin scales as `6 / k` with the loop gain `k` (the second example: 12, 6, 3, 1.2 and, for `k = 7`, 0.857, below one, meaning the closed loop is unstable). When nothing crosses, the margins are infinite and their frequencies are NaN (the third example). The accuracy is set by the frequency grid: use a grid dense around the crossovers.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The three margins of a third-order loop against the exact values.

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

// L(s) = k / (s (s + 1) (s + 2)).
static lti::PoleZero loop_gain(double k = 1.0) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(k)); }
// A logarithmic frequency grid from 0.01 to 100 rad/s.
static pluss::Series grid(int n = 2000) {
  std::vector<double> w;
  for (int i = 0; i < n; ++i) w.push_back(std::pow(10.0, -2.0 + 4.0 * i / (n - 1)));
  return pluss::make_series(w);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // L(s) = 1 / (s (s + 1) (s + 2)): the phase reaches -180 degrees at omega = sqrt(2), where |L| = 1/6.
  const ctl::MarginResult m = ctl::margin(loop_gain(), grid());
  out.writeln("gain margin", m.gain.item<double>(), "at", m.gain_freq.item<double>(), "rad/s; exact 6 at sqrt(2) =", std::sqrt(2.0));
  out.writeln("phase margin", m.phase.item<double>(), "degrees at", m.phase_freq.item<double>(), "rad/s");
  out.writeln("stability margin", m.stability.item<double>(), "at", m.stability_freq.item<double>(), "rad/s");
}
```

Output

```text
gain margin 6.0001 at 1.4142 rad/s; exact 6 at sqrt(2) = 1.4142
phase margin 53.4108 degrees at 0.4457 rad/s
stability margin 0.6376 at 0.7260 rad/s
```
### Example 2

The gain margin as the loop gain grows, until the loop is unstable.

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

// L(s) = k / (s (s + 1) (s + 2)).
static lti::PoleZero loop_gain(double k = 1.0) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(k)); }
// A logarithmic frequency grid from 0.01 to 100 rad/s.
static pluss::Series grid(int n = 2000) {
  std::vector<double> w;
  for (int i = 0; i < n; ++i) w.push_back(std::pow(10.0, -2.0 + 4.0 * i / (n - 1)));
  return pluss::make_series(w);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // The gain margin is the factor by which the loop gain can grow before the loop is unstable: 6 / k here.
  for (const double k : {0.5, 1.0, 2.0, 5.0, 7.0}) {
    const ctl::MarginResult m = ctl::margin(loop_gain(k), grid());
    out.writeln("k =", k, ": gain margin", m.gain.item<double>(), " exact 6 / k =", 6.0 / k, " phase margin", m.phase.item<double>(), "degrees");
  }
}
```

Output

```text
k = 0.5000 : gain margin 12.0001  exact 6 / k = 12.0000  phase margin 69.5563 degrees
k = 1.0000 : gain margin 6.0001  exact 6 / k = 6.0000  phase margin 53.4108 degrees
k = 2.0000 : gain margin 3.0000  exact 6 / k = 3.0000  phase margin 32.6132 degrees
k = 5.0000 : gain margin 1.2000  exact 6 / k = 1.2000  phase margin 5.0230 degrees
k = 7.0000 : gain margin 0.8572  exact 6 / k = 0.8571  phase margin -4.0913 degrees
```
### Example 3

A loop with no crossover.

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

// L(s) = k / (s (s + 1) (s + 2)).
static lti::PoleZero loop_gain(double k = 1.0) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(k)); }
// A logarithmic frequency grid from 0.01 to 100 rad/s.
static pluss::Series grid(int n = 2000) {
  std::vector<double> w;
  for (int i = 0; i < n; ++i) w.push_back(std::pow(10.0, -2.0 + 4.0 * i / (n - 1)));
  return pluss::make_series(w);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // |L| stays below 1: there is no gain crossover and the phase never reaches -180 degrees below the Nyquist-like limit of the grid.
  const lti::PoleZero low = lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({-1.0, -2.0}), pluss::make_series(0.5));
  const ctl::MarginResult m = ctl::margin(low, grid());
  out.writeln("gain margin", m.gain.item<double>(), " phase margin", m.phase.item<double>(), " (infinite: nothing crosses)");
  out.writeln("their frequencies are not numbers:", std::isnan(m.gain_freq.item<double>()), std::isnan(m.phase_freq.item<double>()), "(NaN)");
  out.writeln("stability margin", m.stability.item<double>(), "at", m.stability_freq.item<double>(), "rad/s: the closest approach of L to -1");
}
```

Output

```text
gain margin inf  phase margin inf  (infinite: nothing crosses)
their frequencies are not numbers: true true (NaN)
stability margin 0.9726 at 2.6014 rad/s: the closest approach of L to -1
```
### Example 4

A precomputed response against the system form.

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

// L(s) = k / (s (s + 1) (s + 2)).
static lti::PoleZero loop_gain(double k = 1.0) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(k)); }
// A logarithmic frequency grid from 0.01 to 100 rad/s.
static pluss::Series grid(int n = 2000) {
  std::vector<double> w;
  for (int i = 0; i < n; ++i) w.push_back(std::pow(10.0, -2.0 + 4.0 * i / (n - 1)));
  return pluss::make_series(w);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series omega = grid();
  const lti::PoleZero system = loop_gain();
  const ctl::MarginResult from_system = ctl::margin(system, omega);
  const ctl::MarginResult from_response = ctl::margin(lti::freqresp(system, omega), omega);
  out.writeln("from the system:   gain margin", from_system.gain.item<double>(), " phase margin", from_system.phase.item<double>());
  out.writeln("from the response: gain margin", from_response.gain.item<double>(), " phase margin", from_response.phase.item<double>());
}
```

Output

```text
from the system:   gain margin 6.0001  phase margin 53.4108
from the response: gain margin 6.0001  phase margin 53.4108
```
### Example 5

Every refused grid.

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

// L(s) = k / (s (s + 1) (s + 2)).
static lti::PoleZero loop_gain(double k = 1.0) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0, -1.0, -2.0}), pluss::make_series(k)); }
// A logarithmic frequency grid from 0.01 to 100 rad/s.
static pluss::Series grid(int n = 2000) {
  std::vector<double> w;
  for (int i = 0; i < n; ++i) w.push_back(std::pow(10.0, -2.0 + 4.0 * i / (n - 1)));
  return pluss::make_series(w);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series omega = grid();
  refused(out, "a response and grid of different lengths", [&] { ctl::margin(lti::freqresp(loop_gain(), omega), grid(100)); });
  refused(out, "a frequency grid that does not increase", [&] { ctl::margin(loop_gain(), pluss::make_series({1.0, 0.5, 2.0})); });
  refused(out, "a grid with a single frequency", [&] { ctl::margin(loop_gain(), pluss::make_series({1.0})); });
}
```

Output

```text
a response and grid of different lengths : eng::control::margin: response must have shape [..., frequency]
a frequency grid that does not increase : eng::control::margin: omega must be strictly increasing
a grid with a single frequency : eng::control::margin: omega must be a vector with at least two samples
```