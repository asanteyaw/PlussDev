<!-- generated from loop.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::loop`

Computes the loop response, sensitivity and complementary sensitivity.

## Signature

```cpp
control::LoopResult control::loop(
    const lti::StateSpace& system,
    const pluss::Series& omega);

control::LoopResult control::loop(
    const lti::PoleZero& system,
    const pluss::Series& omega);
```

## What It Does

For an open-loop system `L` it returns `L(jw)`, the sensitivity `S = (I + L)^{-1}` and the complementary sensitivity `T = L (I + L)^{-1}` on a grid of angular frequencies. `S` is the transfer from disturbances at the plant output to the output; `T` is the transfer from the reference, and from sensor noise to the output. The two always sum to the identity. The loop response must be square (as many outputs as inputs).

## Parameters

`system`
: A `StateSpace` or a `PoleZero` open loop.

`omega`
: A non-empty rank-one real floating `Series` of angular frequencies, rad/s.

## Returns

A `LoopResult` with `response`, `sens` and `comp`.

## Errors

A system that is not square (a different number of inputs and outputs) and a scalar, empty or integer `omega` throw `pluss::Error`.

## Interpretation and Use

For `L = 1 / (s (s + 1) (s + 2))` the first example shows the familiar pattern: at low frequency `|L|` is large, so `|S|` is small and `|T|` is near 1; at 10 rad/s `|L|` is 0.001 and the roles reverse. The second example checks `S = 1/(1 + L)`, `T = L/(1 + L)` and `S + T = 1` at 200 frequencies, and the fourth checks `S + T = I` as matrices for a 2 by 2 loop.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The loop response and both sensitivities at three frequencies.

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
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0});
  const ctl::LoopResult r = ctl::loop(loop_gain(), omega);
  const auto l = r.response.items<std::complex<double>>(), s = r.sens.items<std::complex<double>>(), t = r.comp.items<std::complex<double>>();
  for (std::size_t i = 0; i < l.size(); ++i) out.writeln("omega", omega.items<double>()[i], ": |L| =", std::abs(l[i]), " |S| =", std::abs(s[i]), " |T| =", std::abs(t[i]));
}
```

Output

```text
omega 0.1000 : |L| = 4.9690  |S| = 0.2032  |T| = 1.0099
omega 1.0000 : |L| = 0.3162  |S| = 1.4142  |T| = 0.4472
omega 10.0000 : |L| = 0.0010  |S| = 1.0003  |T| = 0.0010
```
### Example 2

The defining identities over a frequency grid.

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
  const pluss::Series omega = grid(200);
  const ctl::LoopResult r = ctl::loop(loop_gain(), omega);
  const auto l = r.response.items<std::complex<double>>(), s = r.sens.items<std::complex<double>>(), t = r.comp.items<std::complex<double>>();
  double worst_s = 0.0, worst_t = 0.0, worst_sum = 0.0;
  for (std::size_t i = 0; i < l.size(); ++i) {
    worst_s = std::max(worst_s, std::abs(s[i] - 1.0 / (1.0 + l[i])));
    worst_t = std::max(worst_t, std::abs(t[i] - l[i] / (1.0 + l[i])));
    worst_sum = std::max(worst_sum, std::abs(s[i] + t[i] - 1.0));
  }
  out.writeln("S against 1 / (1 + L):", worst_s, " T against L / (1 + L):", worst_t, " S + T - 1:", worst_sum, "(largest differences over 200 frequencies)");
}
```

Output

```text
S against 1 / (1 + L): 0.0000  T against L / (1 + L): 0.0000  S + T - 1: 0.0000 (largest differences over 200 frequencies)
```
### Example 3

A pole-zero loop against the same loop as a state-space model.

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
  // 1 / (s + 1) as a pole-zero model and as a state-space model.
  const lti::PoleZero z = lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({-1.0}), pluss::make_series(1.0));
  const lti::StateSpace s = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const pluss::Series omega = pluss::make_series({0.1, 1.0, 10.0});
  const ctl::LoopResult a = ctl::loop(z, omega), b = ctl::loop(s, omega);
  out.writeln("pole-zero response  =", a.response.items<std::complex<double>>());
  out.writeln("state-space response =", b.response.flatten().items<std::complex<double>>());
  out.writeln("largest difference between the two sensitivities =", max_abs(a.sens - b.sens.flatten()));
}
```

Output

```text
pole-zero response  = [(0.990099,-0.0990099), (0.5,-0.5), (0.00990099,-0.0990099)]
state-space response = [(0.990099,-0.0990099), (0.5,-0.5), (0.00990099,-0.0990099)]
largest difference between the two sensitivities = 0.0000
```
### Example 4

A square two-input, two-output loop.

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
  // Two decoupled first-order loops with a coupling: L(s) = C (sI - A)^-1 B for a square 2 x 2 system.
  const lti::StateSpace plant = lti::state(mat({-1.0, 0.0, 0.0, -2.0}, 2, 2), mat({1.0, 0.5, 0.0, 1.0}, 2, 2), mat({1.0, 0.0, 0.0, 1.0}, 2, 2), mat({0.0, 0.0, 0.0, 0.0}, 2, 2));
  const pluss::Series omega = pluss::make_series({0.1, 1.0});
  const ctl::LoopResult r = ctl::loop(plant, omega);
  out.writeln("response shape", r.response.size(0), "x", r.response.size(1), "x", r.response.size(2), " (outputs by inputs by frequencies)");
  const auto s = r.sens.flatten().items<std::complex<double>>(), tt = r.comp.flatten().items<std::complex<double>>();
  double worst = 0.0;
  for (std::int64_t o = 0; o < 2; ++o)
    for (std::int64_t k = 0; k < 2; ++k)
      for (std::int64_t f = 0; f < 2; ++f) worst = std::max(worst, std::abs(s[(o * 2 + k) * 2 + f] + tt[(o * 2 + k) * 2 + f] - (o == k ? 1.0 : 0.0)));
  out.writeln("largest entry of S + T - I =", worst, "(the identity S + T = I holds as matrices)");
}
```

Output

```text
response shape 2 x 2 x 2  (outputs by inputs by frequencies)
largest entry of S + T - I = 0.0000 (the identity S + T = I holds as matrices)
```
### Example 5

Every refused input.

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
  const lti::StateSpace wide = lti::state(pluss::series::eye(2, pluss::ScalarType::Float64).mul(-1.0), pluss::series::ones({2, 2}, pluss::ScalarType::Float64), pluss::series::ones({1, 2}, pluss::ScalarType::Float64), pluss::series::zeros({1, 2}, pluss::ScalarType::Float64));
  refused(out, "two inputs and one output (not square)", [&] { ctl::loop(wide, pluss::make_series({1.0})); });
  refused(out, "a scalar frequency", [&] { ctl::loop(loop_gain(), pluss::make_series(1.0)); });
  refused(out, "an integer frequency grid", [&] { ctl::loop(loop_gain(), pluss::make_series({1, 2})); });
}
```

Output

```text
two inputs and one output (not square) : eng::control::loop: the loop response must be square
a scalar frequency : eng::lti::freqresp: omega must be a non-empty vector
an integer frequency grid : eng::lti::freqresp: omega must have a real floating dtype
```