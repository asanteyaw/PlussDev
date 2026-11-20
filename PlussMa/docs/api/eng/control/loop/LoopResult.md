<!-- generated from LoopResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::LoopResult`

The loop response, sensitivity and complementary sensitivity on a frequency grid.

## Signature

```cpp
control::LoopResult
```

## What It Does

Returned by `loop`. Each field is a complex `Series` with one entry per frequency (and the outputs-by-inputs matrix axes for a multivariable loop).

## Fields

| Field | Meaning |
|---|---|
| `response` | The open-loop response `L(jw)`. |
| `sens` | The sensitivity `S = (I + L)^{-1}`. |
| `comp` | The complementary sensitivity `T = L (I + L)^{-1}`. |

## Returns

Returned by `loop`.

## Errors

Reading a field does not run again.

## Interpretation and Use

Plot or inspect `abs()` of the fields. A peak of `|S|` above one (the second example finds 2.31 for a loop gain of 2) means disturbances are amplified near that frequency; `|T|` near one at low frequency means good tracking and near zero at high frequency means noise rejection (the third example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The shapes and dtype of the fields.

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
  const ctl::LoopResult r = ctl::loop(loop_gain(), grid(5));
  out.writeln("response shape", r.response.numel(), " sensitivity shape", r.sens.numel(), " complementary shape", r.comp.numel(), "(one value per frequency)");
  out.writeln("dtype", r.response.scalar_type());
}
```

Output

```text
response shape 5  sensitivity shape 5  complementary shape 5 (one value per frequency)
dtype complex128
```
### Example 2

The peaks of the two sensitivities for a loop gain of 2.

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
  const ctl::LoopResult r = ctl::loop(loop_gain(2.0), grid());
  const auto s = r.sens.abs().items<double>();
  const auto t = r.comp.abs().items<double>();
  out.writeln("peak of |S| =", *std::max_element(s.begin(), s.end()), " peak of |T| =", *std::max_element(t.begin(), t.end()), "(a peak above 1 means disturbances are amplified near that frequency)");
}
```

Output

```text
peak of |S| = 2.3123  peak of |T| = 1.8371 (a peak above 1 means disturbances are amplified near that frequency)
```
### Example 3

Tracking at low frequency and noise rejection at high frequency.

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
  const ctl::LoopResult r = ctl::loop(loop_gain(2.0), pluss::make_series({0.01, 0.1, 100.0}));
  const auto s = r.sens.abs().items<double>(), t = r.comp.abs().items<double>();
  out.writeln("at 0.01 rad/s |S| =", s[0], " |T| =", t[0], "(good tracking: S small, T near 1)");
  out.writeln("at 100 rad/s |S| =", s[2], " |T| =", t[2], "(noise rejected: T small, S near 1)");
}
```

Output

```text
at 0.01 rad/s |S| = 0.0100  |T| = 1.0001 (good tracking: S small, T near 1)
at 100 rad/s |S| = 1.0000  |T| = 0.0000 (noise rejected: T small, S near 1)
```