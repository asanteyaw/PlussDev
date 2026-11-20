<!-- generated from filter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 885fdcb0e09e -->

# `control::Pid::filter`

[Back to `control::Pid`](../PidController.md)

The derivative-filter time constant.

## Signature

```cpp
const pluss::Series& control::Pid::filter() const noexcept;
```

## What It Does

Returns the time constant `Tf` of the first-order filter on the derivative term, `Kd s / (Tf s + 1)`. A larger value smooths the derivative more and reduces noise amplification at the cost of a slower derivative action. For a discrete controller with a forward derivative rule it must be larger than half the sample time; `pid` rejects combinations that are unstable.

## Returns

A reference to the stored filter time constant.

## Practical Examples

### Example 1

The filter of the example controller is `0.1`.

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
  out.writeln("filter =", controller().filter().item<double>());
}
```

Output

```text
filter = 0.1000
```

### Example 2

A smaller time constant passes more of the derivative.

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
  const ctl::Pid c = ctl::pid(s(2.0), s(1.0), s(0.5), s(0.01));
  out.writeln("filter =", c.filter().item<double>());
}
```

Output

```text
filter = 0.0100
```

### Example 3

A discrete controller keeps its filter beside the sample time.

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
  out.writeln("filter", c.filter().item<double>(), " sample time", c.sample_time());
}
```

Output

```text
filter 0.1000  sample time 0.0200
```

## See Also

- [`kd`](kd.md): the derivative gain it filters
- [`sample_time`](sample_time.md): the sample period

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff_rule.html">
    <span class="label">Previous</span>
    <span class="title">diff_rule</span>
  </a>
  <a class="next-link" href="int_rule.html">
    <span class="label">Next</span>
    <span class="title">int_rule</span>
  </a>
</div>
<!-- apiref:nav:end -->
