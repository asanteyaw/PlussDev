<!-- generated from sample_time.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b448caca1310 -->

# `control::Pid::sample_time`

[Back to `control::Pid`](../PidController.md)

The sample period in seconds; zero for a continuous controller.

## Signature

```cpp
double control::Pid::sample_time() const noexcept;
```

## What It Does

Zero means the controller is continuous, `Kp + Ki/s + Kd s/(Tf s + 1)`. A positive value is the period of a discrete controller, whose integral and derivative are formed with the chosen [`int_rule`](int_rule.md) and [`diff_rule`](diff_rule.md).

## Returns

The sample period in seconds, `0.0` for continuous time.

## Practical Examples

### Example 1

A continuous controller reports zero.

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
  out.writeln("sample time =", controller().sample_time());
}
```

Output

```text
sample time = 0.0000
```

### Example 2

A discrete controller reports its period.

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
  out.writeln("sample time =", controller(0.02).sample_time());
}
```

Output

```text
sample time = 0.0200
```

### Example 3

The period with the two domain tests.

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
  for (const double ts : {0.0, 0.05}) {
    const ctl::Pid c = controller(ts);
    out.writeln("sample time", c.sample_time(), " continuous", c.is_cont(), " discrete", c.is_disc());
  }
}
```

Output

```text
sample time 0.0000  continuous true  discrete false
sample time 0.0500  continuous false  discrete true
```

## See Also

- [`is_cont`](is_cont.md): true when continuous
- [`is_disc`](is_disc.md): true when discrete

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kp.html">
    <span class="label">Previous</span>
    <span class="title">kp</span>
  </a>
</div>
<!-- apiref:nav:end -->
