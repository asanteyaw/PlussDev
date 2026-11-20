<!-- generated from kp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 253464647cfe -->

# `control::Pid::kp`

[Back to `control::Pid`](../PidController.md)

The proportional gain `Kp`.

## Signature

```cpp
const pluss::Series& control::Pid::kp() const noexcept;
```

## What It Does

Returns the proportional gain `Kp` as a `Series`, stored as given to `pid` (promoted to the common floating dtype of the four gains). A single controller holds one element; a batch of controllers holds one element per controller on the leading axes. Read it to check what was configured or to rebuild the controller with one value changed.

## Returns

A reference to the stored proportional gain `Kp`.

## Practical Examples

### Example 1

A controller with `Kp = 2`, `Ki = 1`, `Kd = 0.5`.

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
  const ctl::Pid c = controller();
  out.writeln("kp =", c.kp().item<double>());
}
```

Output

```text
kp = 2.0000
```

### Example 2

A batch of three proportional gains builds three controllers at once.

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
  const ctl::Pid c = ctl::pid(pluss::make_series({1.0, 2.0, 4.0}), pluss::make_series({0.0, 0.0, 0.0}), pluss::make_series({0.0, 0.0, 0.0}), pluss::make_series({0.1, 0.1, 0.1}));
  out.writeln("kp =", c.kp().items<double>());
}
```

Output

```text
kp = [1.0000, 2.0000, 4.0000]
```

### Example 3

A pure integral controller has `Kp = 0`.

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
  const ctl::Pid c = ctl::pid(s(0.0), s(3.0), s(0.0), s(0.1));
  out.writeln("kp =", c.kp().item<double>(), " ki =", c.ki().item<double>());
}
```

Output

```text
kp = 0.0000  ki = 3.0000
```

## See Also

- [`kp`](kp.md): the proportional gain
- [`ki`](ki.md): the integral gain
- [`kd`](kd.md): the derivative gain

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ki.html">
    <span class="label">Previous</span>
    <span class="title">ki</span>
  </a>
  <a class="next-link" href="sample_time.html">
    <span class="label">Next</span>
    <span class="title">sample_time</span>
  </a>
</div>
<!-- apiref:nav:end -->
