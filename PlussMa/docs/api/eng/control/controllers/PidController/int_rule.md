<!-- generated from int_rule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f2dfd51d86aa -->

# `control::Pid::int_rule`

[Back to `control::Pid`](../PidController.md)

The rule used to form the integral of a discrete controller.

## Signature

```cpp
control::PidRule control::Pid::int_rule() const noexcept;
```

## What It Does

Returns the `PidRule` that replaces `1/s` in the integral term: `Forward`, `Backward` or `Trapezoid`. The rule only matters when the controller is discrete; a continuous controller stores it but never uses it. Trapezoid is the most accurate for a given period; forward is the cheapest.

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
  out.writeln("forward:", controller(0.02).int_rule() == ctl::PidRule::Forward);
}
```

Output

```text
forward: true
```

### Example 2

Request the trapezoid rule for the integral.

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
  const ctl::Pid c = controller(0.02, ctl::PidRule::Trapezoid);
  out.writeln("trapezoid:", c.int_rule() == ctl::PidRule::Trapezoid);
}
```

Output

```text
trapezoid: true
```

### Example 3

The integral and derivative rules are stored independently.

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
  const ctl::Pid c = controller(0.02, ctl::PidRule::Backward, ctl::PidRule::Forward);
  out.writeln("integral is backward:", c.int_rule() == ctl::PidRule::Backward, " derivative is backward:", c.diff_rule() == ctl::PidRule::Backward);
}
```

Output

```text
integral is backward: true  derivative is backward: false
```

## See Also

- [`diff_rule`](diff_rule.md): the derivative rule
- [`sample_time`](sample_time.md): the sample period

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="is_cont.html">
    <span class="label">Next</span>
    <span class="title">is_cont</span>
  </a>
</div>
<!-- apiref:nav:end -->
