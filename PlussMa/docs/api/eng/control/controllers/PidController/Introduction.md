<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `control::Pid` Introduction

Holds the four gains, the sample time and the two discrete rules of a PID controller. It is created only through `pid`, which validates the combination, and consumed by `control::freqresp`.

The gains are stored promoted to one common dtype (the second example gives a float32 and three float64 gains and reads back float64 for all four). The rules are meaningful only for a discrete controller.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every accessor.

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
  const ctl::Pid c = ctl::pid(s(2.0), s(1.0), s(0.5), s(0.1), 0.02, ctl::PidRule::Trapezoid, ctl::PidRule::Backward);
  out.writeln("kp", c.kp().item<double>(), "ki", c.ki().item<double>(), "kd", c.kd().item<double>(), "filter", c.filter().item<double>());
  out.writeln("sample time", c.sample_time(), " discrete:", c.is_disc(), " continuous:", c.is_cont());
  out.writeln("integral rule is Trapezoid:", c.int_rule() == ctl::PidRule::Trapezoid, " derivative rule is Backward:", c.diff_rule() == ctl::PidRule::Backward);
}
```

Output

```text
kp 2.0000 ki 1.0000 kd 0.5000 filter 0.1000
sample time 0.0200  discrete: true  continuous: false
integral rule is Trapezoid: true  derivative rule is Backward: true
```
### Example 2

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
### Example 3

Construction errors seen through the type.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="diff_rule.html">
    <span class="label">Next</span>
    <span class="title">diff_rule</span>
  </a>
</div>
<!-- apiref:nav:end -->
