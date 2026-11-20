<!-- generated from horizon.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d4931113961 -->

# `control::mpc::Plan::horizon`

[Back to `control::mpc::Plan`](../Plan.md)

The prediction horizon the plan was built with.

## Signature

```cpp
std::int64_t control::mpc::Plan::horizon() const noexcept;
```

## What It Does

Returns the number of steps `N` the plan looks ahead. A solve returns `N` input moves; the cost of building the plan grows with `N`, and so does the quality of the predicted trajectory. It equals `Options::horizon` given to `build`.

## Returns

The number of prediction steps.

## Practical Examples

### Example 1

A plan built for 6 steps.

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
  const mpc::Plan p = mpc::build(integrator(), weights1(), limits(), horizon(6));
  out.writeln("horizon =", p.horizon());
}
```

Output

```text
horizon = 6
```

### Example 2

A longer horizon on the same system.

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
  const mpc::Plan p = mpc::build(integrator(), weights1(), limits(), horizon(20));
  out.writeln("horizon =", p.horizon());
}
```

Output

```text
horizon = 20
```

### Example 3

The horizon does not change the number of states or inputs.

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
  for (const std::int64_t n : {3, 12}) {
    const mpc::Plan p = mpc::build(integrator(), weights1(), limits(), horizon(n));
    out.writeln("horizon", p.horizon(), " states", p.states(), " inputs", p.inputs());
  }
}
```

Output

```text
horizon 3  states 1  inputs 1
horizon 12  states 1  inputs 1
```

## See Also

- [`states`](states.md): the number of states
- [`inputs`](inputs.md): the number of inputs

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtype.html">
    <span class="label">Previous</span>
    <span class="title">dtype</span>
  </a>
  <a class="next-link" href="inputs.html">
    <span class="label">Next</span>
    <span class="title">inputs</span>
  </a>
</div>
<!-- apiref:nav:end -->
