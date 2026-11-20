<!-- generated from freqresp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::freqresp`

Evaluates the frequency response of a PID controller.

## Signature

```cpp
pluss::Series control::freqresp(
    const control::Pid& controller,
    const pluss::Series& omega);
```

## What It Does

Returns `C(j w)` for a continuous controller, or `C(e^{j w T})` with the chosen rules for a discrete one, for each angular frequency `w` in rad/s. A batch of controllers gives one row per controller.

## Parameters

`controller`
: A `Pid`.

`omega`
: A finite floating rank-one `Series` of angular frequencies, rad/s.

## Returns

A complex `Series` with the controller batch axes followed by the frequency axis.

## Errors

A scalar, non-vector, integer or non-finite `omega`, and a controller and grid on different platforms, throw `pluss::Error`.

## Interpretation and Use

Each term is separable: with only `Kp` the response is the constant gain, with only `Ki` it is `-j Ki / w`, with only `Kd` it is `j Kd w` for an unfiltered derivative (the first example). The second example compares the discrete integrator under each rule with the ideal at `omega = 10`.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Each term of the controller on its own.

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
  const pluss::Series omega = pluss::make_series({0.5, 2.0});
  const auto only_p = ctl::freqresp(ctl::pid(s(3.0), s(0.0), s(0.0), s(0.0)), omega).items<std::complex<double>>();
  const auto only_i = ctl::freqresp(ctl::pid(s(0.0), s(3.0), s(0.0), s(0.0)), omega).items<std::complex<double>>();
  const auto only_d = ctl::freqresp(ctl::pid(s(0.0), s(0.0), s(3.0), s(0.0)), omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < 2; ++i) {
    const double w = omega.items<double>()[i];
    out.writeln("omega", w, ": P =", only_p[i], " I =", only_i[i], " (3 / j omega =", std::complex<double>(0.0, -3.0 / w), ") D =", only_d[i]);
  }
}
```

Output

```text
omega 0.5000 : P = (3,0)  I = (0,-6)  (3 / j omega = (0,-6) ) D = (0,1.5)
omega 2.0000 : P = (3,0)  I = (0,-1.5)  (3 / j omega = (0,-1.5) ) D = (0,6)
```
### Example 2

A discrete integrator under each rule against the ideal.

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
  const pluss::Series omega = pluss::make_series({10.0});
  const std::complex<double> ideal = ctl::freqresp(ctl::pid(s(0.0), s(1.0), s(0.0), s(0.0)), omega).items<std::complex<double>>()[0];
  out.writeln("ideal integrator 1 / (j omega) at omega = 10: magnitude", std::abs(ideal), " phase", std::arg(ideal));
  for (const auto [label, rule] : {std::pair{"Forward  ", ctl::PidRule::Forward}, {"Backward ", ctl::PidRule::Backward}, {"Trapezoid", ctl::PidRule::Trapezoid}}) {
    const std::complex<double> g = ctl::freqresp(ctl::pid(s(0.0), s(1.0), s(0.0), s(0.0), 0.05, rule, ctl::PidRule::Backward), omega).items<std::complex<double>>()[0];
    out.writeln(label, "sample time 0.05: magnitude", std::abs(g), " phase", std::arg(g));
  }
}
```

Output

```text
ideal integrator 1 / (j omega) at omega = 10: magnitude 0.1000  phase -1.5708
Forward   sample time 0.05: magnitude 0.1010  phase -1.8208
Backward  sample time 0.05: magnitude 0.1010  phase -1.3208
Trapezoid sample time 0.05: magnitude 0.0979  phase -1.5708
```
### Example 3

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto s = [](double v) { return pluss::make_series(v); };
  const ctl::Pid c = ctl::pid(s(1.0), s(1.0), s(0.0), s(0.0));
  refused(out, "a scalar frequency", [&] { ctl::freqresp(c, pluss::make_series(1.0)); });
  refused(out, "an integer frequency grid", [&] { ctl::freqresp(c, pluss::make_series({1, 2})); });
  refused(out, "a non-finite frequency", [&] { ctl::freqresp(c, pluss::make_series({1.0, std::nan("")})); });
}
```

Output

```text
a scalar frequency : eng::control::freqresp(Pid): omega must be a defined vector
an integer frequency grid : eng::control::freqresp(Pid): omega must be finite and floating
a non-finite frequency : eng::control::freqresp(Pid): omega must be finite and floating
```