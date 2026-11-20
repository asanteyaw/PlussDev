<!-- generated from gain.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6453d7b3ea33 -->

# `lti::PoleZero::gain`

[Back to `lti::PoleZero`](../PoleZero.md)

The scalar gain of the zero-pole-gain form.

## Signature

```cpp
const pluss::Series& lti::PoleZero::gain() const noexcept;
```

## What It Does

Returns the gain `k` of `G = k (s - z1)... / ((s - p1)...)`. It is a `Series` (so it can be batched) and for a single system a one-element series; take `.real()` for a real system. The gain scales the whole response without changing where the poles and zeros are.

## Returns

A reference to the stored gain.

## Practical Examples

### Example 1

The plant gain is `5`.

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
  out.writeln("gain =", plant().gain().real().item<double>());
}
```

Output

```text
gain = 5.0000
```

### Example 2

Rescaling the gain leaves the poles unchanged.

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
  const lti::PoleZero g = lti::zpk(pluss::make_series({-1.0}), pluss::make_series({-2.0, -3.0}), pluss::make_series(0.5));
  out.writeln("gain =", g.gain().real().item<double>(), " poles =", g.poles().items<double>());
}
```

Output

```text
gain = 0.5000  poles = [-2.0000, -3.0000]
```

### Example 3

A negative gain is allowed and flips the sign of the response.

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
  const lti::PoleZero g = lti::zpk(pluss::make_series({-1.0}), pluss::make_series({-2.0}), pluss::make_series(-3.0));
  out.writeln("gain =", g.gain().real().item<double>());
}
```

Output

```text
gain = -3.0000
```

## See Also

- [`zeros`](zeros.md): the zeros
- [`poles`](poles.md): the poles

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="is_cont.html">
    <span class="label">Next</span>
    <span class="title">is_cont</span>
  </a>
</div>
<!-- apiref:nav:end -->
