<!-- generated from inputs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 492fc5dbe62f -->

# `control::mpc::Plan::inputs`

[Back to `control::mpc::Plan`](../Plan.md)

The number of inputs of the planned system.

## Signature

```cpp
std::int64_t control::mpc::Plan::inputs() const noexcept;
```

## What It Does

Returns `m`, the input dimension. Each solve returns a plan of `horizon() * inputs()` input values, arranged `N x m`.

## Returns

The number of inputs.

## Practical Examples

### Example 1

Both example systems have a single input.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(std::move(v), {r, c}); }
// A discrete integrator x[k+1] = x[k] + u[k].
static lti::StateSpace integrator() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// A discrete double integrator with a 0.1 s step: position and velocity, one force input.
static lti::StateSpace cart() { return lti::state(mat({1.0, 0.1, 0.0, 1.0}, 2, 2), mat({0.005, 0.1}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), 0.1); }
static mpc::Weights weights1() { return {mat({1.0}, 1, 1), mat({0.1}, 1, 1), mat({2.0}, 1, 1)}; }
static mpc::Weights weights2() { return {mat({1.0, 0.0, 0.0, 1.0}, 2, 2), mat({0.1}, 1, 1), mat({2.0, 0.0, 0.0, 2.0}, 2, 2)}; }
static mpc::Bounds limits() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options horizon(std::int64_t n) { mpc::Options o; o.horizon = n; return o; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("inputs =", mpc::build(cart(), weights2(), limits(), horizon(4)).inputs());
}
```

Output

```text
inputs = 1
```

### Example 2

The input count is independent of the horizon.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(std::move(v), {r, c}); }
// A discrete integrator x[k+1] = x[k] + u[k].
static lti::StateSpace integrator() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// A discrete double integrator with a 0.1 s step: position and velocity, one force input.
static lti::StateSpace cart() { return lti::state(mat({1.0, 0.1, 0.0, 1.0}, 2, 2), mat({0.005, 0.1}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), 0.1); }
static mpc::Weights weights1() { return {mat({1.0}, 1, 1), mat({0.1}, 1, 1), mat({2.0}, 1, 1)}; }
static mpc::Weights weights2() { return {mat({1.0, 0.0, 0.0, 1.0}, 2, 2), mat({0.1}, 1, 1), mat({2.0, 0.0, 0.0, 2.0}, 2, 2)}; }
static mpc::Bounds limits() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options horizon(std::int64_t n) { mpc::Options o; o.horizon = n; return o; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const std::int64_t n : {2, 8}) out.writeln("horizon", n, " inputs", mpc::build(integrator(), weights1(), limits(), horizon(n)).inputs());
}
```

Output

```text
horizon 2  inputs 1
horizon 8  inputs 1
```

### Example 3

A plan reports all three sizes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
namespace mpc = pluss::subsp::eng::control::mpc;
using namespace pluss::tils;

static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(std::move(v), {r, c}); }
// A discrete integrator x[k+1] = x[k] + u[k].
static lti::StateSpace integrator() { return lti::state(mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 1.0); }
// A discrete double integrator with a 0.1 s step: position and velocity, one force input.
static lti::StateSpace cart() { return lti::state(mat({1.0, 0.1, 0.0, 1.0}, 2, 2), mat({0.005, 0.1}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1), 0.1); }
static mpc::Weights weights1() { return {mat({1.0}, 1, 1), mat({0.1}, 1, 1), mat({2.0}, 1, 1)}; }
static mpc::Weights weights2() { return {mat({1.0, 0.0, 0.0, 1.0}, 2, 2), mat({0.1}, 1, 1), mat({2.0, 0.0, 0.0, 2.0}, 2, 2)}; }
static mpc::Bounds limits() { return {pluss::make_series({-10.0}), pluss::make_series({10.0})}; }
static mpc::Options horizon(std::int64_t n) { mpc::Options o; o.horizon = n; return o; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const mpc::Plan p = mpc::build(cart(), weights2(), limits(), horizon(5));
  out.writeln("horizon", p.horizon(), " states", p.states(), " inputs", p.inputs());
}
```

Output

```text
horizon 5  states 2  inputs 1
```

## See Also

- [`states`](states.md): the number of states
- [`horizon`](horizon.md): the prediction horizon

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="horizon.html">
    <span class="label">Previous</span>
    <span class="title">horizon</span>
  </a>
  <a class="next-link" href="platform.html">
    <span class="label">Next</span>
    <span class="title">platform</span>
  </a>
</div>
<!-- apiref:nav:end -->
