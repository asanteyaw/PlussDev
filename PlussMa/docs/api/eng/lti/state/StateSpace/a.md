<!-- generated from a.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5682b63b4b83 -->

# `lti::StateSpace::a`

[Back to `lti::StateSpace`](../StateSpace.md)

The state matrix `A` exactly as it was given to `state`.

## Signature

```cpp
const pluss::Series& lti::StateSpace::a() const noexcept;
```

## What It Does

Returns `A`, the state matrix, with the shape it was built with: `n x n` for `n` states. The system stores the four matrices unchanged, so reading one never copies or rearranges data. Call it when you need the matrix itself, for a hand calculation, a plot or a comparison with another model; call [`states`](states.md), [`inputs`](inputs.md) or [`outputs`](outputs.md) when only a dimension matters. When the system was built with batch axes, those leading axes are present here too.

## Returns

A reference to the stored `A` as a `pluss::Series`. It stays valid for the life of the `StateSpace`.

## Practical Examples

### Example 1

Read `A` of an oscillator and print its entries and its shape.

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
  out.writeln("A =", s.a().flatten().items<double>());
  out.writeln("shape:", s.a().size(0), "x", s.a().size(1));
}
```

Output

```text
A = [0.0000, 1.0000, -4.0000, -0.4000]
shape: 2 x 2
```

### Example 2

The eigenvalues of `A` decide stability: for this oscillator the trace of `A` is negative, so the motion decays.

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
  const double trace = s.a().select(0, 0).items<double>()[0] + s.a().select(0, 1).items<double>()[1];
  out.writeln("trace of A =", trace);
}
```

Output

```text
trace of A = -0.4000
```

### Example 3

Changing one entry of the data you pass builds a different system; the stored `A` reflects it.

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
  const lti::StateSpace stiff = lti::state(mat({0.0, 1.0, -9.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
  out.writeln("lower-left entry:", stiff.a().select(0, 1).items<double>()[0]);
}
```

Output

```text
lower-left entry: -9.0000
```

## See Also

- [`states`](states.md): the number of states
- [`inputs`](inputs.md): the number of inputs
- [`outputs`](outputs.md): the number of outputs

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="b.html">
    <span class="label">Next</span>
    <span class="title">b</span>
  </a>
</div>
<!-- apiref:nav:end -->
