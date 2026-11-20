<!-- generated from states.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 47c3eff5204e -->

# `lti::StateSpace::states`

[Back to `lti::StateSpace`](../StateSpace.md)

The number of states, `n`.

## Signature

```cpp
std::int64_t lti::StateSpace::states() const;
```

## What It Does

Returns `n`, the number of states, read from the last dimension of `A`. It is the quickest way to size a vector or loop that accompanies the system without taking a matrix apart. The value is an integer count and does not depend on any batch axes.

## Returns

`n` as a non-negative integer.

## Practical Examples

### Example 1

The oscillator has two states.

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
  out.writeln("states =", oscillator().states());
}
```

Output

```text
states = 2
```

### Example 2

A first-order lag has one state.

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
  const lti::StateSpace s = lti::state(mat({-2.0}, 1, 1), mat({2.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  out.writeln("states =", s.states());
}
```

Output

```text
states = 1
```

### Example 3

`states()` always matches the sides of `A`.

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
  const lti::StateSpace s = oscillator();
  out.writeln("states =", s.states(), ", A is", s.a().size(0), "x", s.a().size(1));
}
```

Output

```text
states = 2 , A is 2 x 2
```

## See Also

- [`inputs`](inputs.md): the number of inputs
- [`outputs`](outputs.md): the number of outputs
- [`a`](a.md): the matrix whose size it reads

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sample_time.html">
    <span class="label">Previous</span>
    <span class="title">sample_time</span>
  </a>
</div>
<!-- apiref:nav:end -->
