<!-- generated from is_disc.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 21159bba5382 -->

# `lti::StateSpace::is_disc`

[Back to `lti::StateSpace`](../StateSpace.md)

Whether the system is discrete-time, that is, its sample time is positive.

## Signature

```cpp
bool lti::StateSpace::is_disc() const noexcept;
```

## What It Does

True when `sample_time()` is positive, so `A` generates `x[k+1] = A x[k] + B u[k]`. It is the negation of [`is_cont`](is_cont.md). Check it before treating the eigenvalues of `A` as discrete poles, which must lie inside the unit circle.

## Returns

`true` for a discrete-time system, `false` otherwise.

## Practical Examples

### Example 1

A system built without a sample time is not discrete.

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
  out.writeln("discrete:", oscillator().is_disc());
}
```

Output

```text
discrete: false
```

### Example 2

Giving a period makes it discrete.

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
  out.writeln("discrete:", oscillator(0.02).is_disc());
}
```

Output

```text
discrete: true
```

### Example 3

The two tests are complementary.

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
  for (const double ts : {0.0, 0.5}) {
    const lti::StateSpace s = oscillator(ts);
    out.writeln("cont", s.is_cont(), " disc", s.is_disc());
  }
}
```

Output

```text
cont true  disc false
cont false  disc true
```

## See Also

- [`is_cont`](is_cont.md): its negation
- [`sample_time`](sample_time.md): the period itself

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_cont.html">
    <span class="label">Previous</span>
    <span class="title">is_cont</span>
  </a>
  <a class="next-link" href="outputs.html">
    <span class="label">Next</span>
    <span class="title">outputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
