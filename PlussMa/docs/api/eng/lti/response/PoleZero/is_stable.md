<!-- generated from is_stable.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 28c8de6601c5 -->

# `lti::PoleZero::is_stable`

[Back to `lti::PoleZero`](../PoleZero.md)

Whether every pole lies in the stable region for the system's time domain.

## Signature

```cpp
bool lti::PoleZero::is_stable() const;
```

## What It Does

True when every pole is strictly inside the stable region: the open left half-plane for a continuous system, the open unit disc for a discrete one. A pole on the boundary, such as an integrator at zero or a pole of magnitude one, counts as not stable. Zeros do not matter. Use it to screen a design before simulating it.

## Returns

`true` when all poles are strictly inside the stable region, `false` otherwise.

## Practical Examples

### Example 1

Poles at `-2` and `-3` are stable.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

// G(s) = 5 (s + 1) / ((s + 2)(s + 3))
static lti::PoleZero plant(double sample_time = 0.0) { return lti::zpk(pluss::make_series({-1.0}), pluss::make_series({-2.0, -3.0}), pluss::make_series(5.0), sample_time); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("stable:", plant().is_stable());
}
```

Output

```text
stable: true
```

### Example 2

An integrator pole at zero is on the boundary and is not stable.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

// G(s) = 5 (s + 1) / ((s + 2)(s + 3))
static lti::PoleZero plant(double sample_time = 0.0) { return lti::zpk(pluss::make_series({-1.0}), pluss::make_series({-2.0, -3.0}), pluss::make_series(5.0), sample_time); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::PoleZero g = lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({0.0}), pluss::make_series(1.0));
  out.writeln("stable:", g.is_stable());
}
```

Output

```text
stable: false
```

### Example 3

The same pole value is judged differently in each domain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

// G(s) = 5 (s + 1) / ((s + 2)(s + 3))
static lti::PoleZero plant(double sample_time = 0.0) { return lti::zpk(pluss::make_series({-1.0}), pluss::make_series({-2.0, -3.0}), pluss::make_series(5.0), sample_time); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto make = [](double pole, double ts) { return lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({pole}), pluss::make_series(1.0), ts); };
  out.writeln("pole -0.5, continuous:", make(-0.5, 0.0).is_stable());
  out.writeln("pole  1.5, discrete:  ", make(1.5, 0.1).is_stable());
  out.writeln("pole  0.5, discrete:  ", make(0.5, 0.1).is_stable());
}
```

Output

```text
pole -0.5, continuous: true
pole  1.5, discrete:   false
pole  0.5, discrete:   true
```

## See Also

- [`poles`](poles.md): the poles that are tested

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_disc.html">
    <span class="label">Previous</span>
    <span class="title">is_disc</span>
  </a>
  <a class="next-link" href="poles.html">
    <span class="label">Next</span>
    <span class="title">poles</span>
  </a>
</div>
<!-- apiref:nav:end -->
