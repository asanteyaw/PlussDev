<!-- generated from diff_rule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af339e3e76df -->

# `control::Pid::diff_rule`

[Back to `control::Pid`](../PidController.md)

The rule used to form the filtered derivative of a discrete controller.

## Signature

```cpp
control::PidRule control::Pid::diff_rule() const noexcept;
```

## What It Does

Returns the `PidRule` used for the filtered derivative `Kd s/(Tf s + 1)`. Only a discrete controller uses it. A trapezoidal derivative without a filter is unstable and is rejected by `pid`; a forward derivative needs `sample_time < 2 Tf`.

## Returns

The stored `PidRule`.

## Practical Examples

### Example 1

The default is the forward rule.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

static pluss::Series s(double v) { return pluss::make_series(v); }
static ctl::Pid controller(double ts = 0.0, ctl::PidRule i = ctl::PidRule::Forward, ctl::PidRule d = ctl::PidRule::Forward) {
  return ctl::pid(s(2.0), s(1.0), s(0.5), s(0.1), ts, i, d);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("forward:", controller(0.02).diff_rule() == ctl::PidRule::Forward);
}
```

Output

```text
forward: true
```

### Example 2

Request the backward rule for the derivative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

static pluss::Series s(double v) { return pluss::make_series(v); }
static ctl::Pid controller(double ts = 0.0, ctl::PidRule i = ctl::PidRule::Forward, ctl::PidRule d = ctl::PidRule::Forward) {
  return ctl::pid(s(2.0), s(1.0), s(0.5), s(0.1), ts, i, d);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ctl::Pid c = controller(0.02, ctl::PidRule::Forward, ctl::PidRule::Backward);
  out.writeln("backward:", c.diff_rule() == ctl::PidRule::Backward);
}
```

Output

```text
backward: true
```

### Example 3

Both rules read back together.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace ctl = pluss::subsp::eng::control;
using namespace pluss::tils;

static pluss::Series s(double v) { return pluss::make_series(v); }
static ctl::Pid controller(double ts = 0.0, ctl::PidRule i = ctl::PidRule::Forward, ctl::PidRule d = ctl::PidRule::Forward) {
  return ctl::pid(s(2.0), s(1.0), s(0.5), s(0.1), ts, i, d);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const ctl::Pid c = controller(0.02, ctl::PidRule::Trapezoid, ctl::PidRule::Backward);
  out.writeln("integral trapezoid:", c.int_rule() == ctl::PidRule::Trapezoid, " derivative backward:", c.diff_rule() == ctl::PidRule::Backward);
}
```

Output

```text
integral trapezoid: true  derivative backward: true
```

## See Also

- [`int_rule`](int_rule.md): the integral rule
- [`filter`](filter.md): the derivative filter

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
<!-- apiref:nav:end -->
