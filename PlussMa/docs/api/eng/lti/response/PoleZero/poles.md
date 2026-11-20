<!-- generated from poles.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dedfb2d73a01 -->

# `lti::PoleZero::poles`

[Back to `lti::PoleZero`](../PoleZero.md)

The poles of the transfer function.

## Signature

```cpp
const pluss::Series& lti::PoleZero::poles() const noexcept;
```

## What It Does

Returns the poles, the values at which the transfer function grows without bound, one entry per pole on the last axis. Their locations decide stability: see [`is_stable`](is_stable.md). Read them to compare designs, for example how far a filter's poles sit from the unit circle.

## Returns

A reference to the stored poles.

## Practical Examples

### Example 1

The plant has poles at `-2` and `-3`.

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
  out.writeln("poles =", plant().poles().items<double>());
}
```

Output

```text
poles = [-2.0000, -3.0000]
```

### Example 2

The number of poles is the order of the denominator.

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
  out.writeln("order =", plant().poles().numel());
}
```

Output

```text
order = 2
```

### Example 3

A filter designed by `signal::butter` carries its poles; for order 2 there are two.

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
  namespace sig = pluss::subsp::eng::signal;
  const lti::PoleZero f = sig::butter(2, 0.3);
  out.writeln("poles:", f.poles().numel(), " stable:", f.is_stable());
}
```

Output

```text
poles: 2  stable: true
```

## See Also

- [`zeros`](zeros.md): the zeros
- [`is_stable`](is_stable.md): whether the poles are in the stable region

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_stable.html">
    <span class="label">Previous</span>
    <span class="title">is_stable</span>
  </a>
  <a class="next-link" href="sample_time.html">
    <span class="label">Next</span>
    <span class="title">sample_time</span>
  </a>
</div>
<!-- apiref:nav:end -->
