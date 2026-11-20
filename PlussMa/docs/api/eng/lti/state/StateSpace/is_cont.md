<!-- generated from is_cont.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5689923663f8 -->

# `lti::StateSpace::is_cont`

[Back to `lti::StateSpace`](../StateSpace.md)

Whether the system is continuous-time, that is, its sample time is zero.

## Signature

```cpp
bool lti::StateSpace::is_cont() const noexcept;
```

## What It Does

True when `sample_time()` is zero, so `A` generates `x' = A x + B u`. Use it to branch between continuous and discrete handling instead of comparing the sample time yourself. It is the negation of [`is_disc`](is_disc.md).

## Returns

`true` for a continuous-time system, `false` otherwise.

## Practical Examples

### Example 1

The default is continuous.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(std::move(v), {r, c}); }
// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator(double sample_time = 0.0) {
  return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), sample_time);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("continuous:", oscillator().is_cont());
}
```

Output

```text
continuous: true
```

### Example 2

A positive sample time makes it discrete.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(std::move(v), {r, c}); }
// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator(double sample_time = 0.0) {
  return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), sample_time);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("continuous:", oscillator(0.1).is_cont());
}
```

Output

```text
continuous: false
```

### Example 3

Choose a reading of the eigenvalues by domain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(std::move(v), {r, c}); }
// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator(double sample_time = 0.0) {
  return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), sample_time);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const double ts : {0.0, 0.1}) {
    const lti::StateSpace s = oscillator(ts);
    out.writeln(s.is_cont() ? "stable if the eigenvalues have negative real part" : "stable if the eigenvalues lie inside the unit circle");
  }
}
```

Output

```text
stable if the eigenvalues have negative real part
stable if the eigenvalues lie inside the unit circle
```

## See Also

- [`is_disc`](is_disc.md): its negation
- [`sample_time`](sample_time.md): the period itself

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inputs.html">
    <span class="label">Previous</span>
    <span class="title">inputs</span>
  </a>
  <a class="next-link" href="is_disc.html">
    <span class="label">Next</span>
    <span class="title">is_disc</span>
  </a>
</div>
<!-- apiref:nav:end -->
