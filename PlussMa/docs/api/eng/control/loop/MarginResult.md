<!-- generated from MarginResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::MarginResult`

The gain, phase and stability margins with their frequencies.

## Signature

```cpp
control::MarginResult
```

## What It Does

Returned by `margin`. Each margin is paired with the frequency at which it occurs; infinity means the corresponding crossover does not exist on the grid.

## Fields

| Field | Meaning |
|---|---|
| `gain` | Gain margin: a linear factor, greater than one for a stable loop. |
| `phase` | Phase margin in degrees. |
| `stability` | The smallest `|1 + L|` over the grid. |
| `gain_freq` | Frequency of the gain margin (the phase crossover), rad/s. |
| `phase_freq` | Frequency of the phase margin (the gain crossover), rad/s. |
| `stability_freq` | Frequency of the closest approach to `-1`, rad/s. |

## Returns

Returned by `margin`.

## Errors

Reading a field does not run again.

## Interpretation and Use

A stable loop has a gain margin above one and a positive phase margin; the second example of `margin` shows both turning bad together as the gain grows (a negative phase margin at `k = 7`). The stability margin is the more honest robustness number: it is the minimum of `|1 + L|` over the grid, which the second example here reproduces independently (0.6376). As a common rule of thumb, look for a gain margin above 2, a phase margin above 30 degrees and a stability margin above about 0.5.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The six fields for a third-order loop.

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
  const ctl::MarginResult m = ctl::margin(loop_gain(), grid());
  out.writeln("gain", m.gain.item<double>(), "at", m.gain_freq.item<double>());
  out.writeln("phase", m.phase.item<double>(), "at", m.phase_freq.item<double>());
  out.writeln("stability", m.stability.item<double>(), "at", m.stability_freq.item<double>());
}
```

Output

```text
gain 6.0001 at 1.4142
phase 53.4108 at 0.4457
stability 0.6376 at 0.7260
```
### Example 2

The stability margin against the minimum of `|1 + L|`.

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
  // The stability margin is the closest approach of the Nyquist curve to -1: the smallest |1 + L|.
  const pluss::Series omega = grid();
  const pluss::Series l = lti::freqresp(loop_gain(), omega);
  const auto d = l.add(1.0).abs().items<double>();
  const ctl::MarginResult m = ctl::margin(l, omega);
  out.writeln("minimum of |1 + L| over the grid =", *std::min_element(d.begin(), d.end()), " reported stability margin =", m.stability.item<double>());
}
```

Output

```text
minimum of |1 + L| over the grid = 0.6376  reported stability margin = 0.6376
```
### Example 3

A batch of three loops with different gains.

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
  // A batch of loops: one margin per loop.
  std::vector<std::complex<double>> rows;
  const pluss::Series omega = grid(400);
  const pluss::Series a = lti::freqresp(loop_gain(0.5), omega), b = lti::freqresp(loop_gain(1.0), omega), c = lti::freqresp(loop_gain(2.0), omega);
  const ctl::MarginResult m = ctl::margin(pluss::series::pile({a, b, c}, 0), omega);
  out.writeln("gain margins of k = 0.5, 1, 2:", m.gain.items<double>(), " exact 12, 6, 3");
}
```

Output

```text
gain margins of k = 0.5, 1, 2: [12.0046, 6.0023, 3.0012]  exact 12, 6, 3
```