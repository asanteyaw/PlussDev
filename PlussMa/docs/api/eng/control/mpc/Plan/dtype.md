<!-- generated from dtype.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9aaf30c9069c -->

# `control::mpc::Plan::dtype`

[Back to `control::mpc::Plan`](../Plan.md)

The floating dtype of the plan's solver.

## Signature

```cpp
pluss::ScalarType control::mpc::Plan::dtype() const noexcept;
```

## What It Does

Returns the floating-point type the plan solves in, taken from the system and weights. Initial states and references are converted to it when you solve.

## Returns

The `ScalarType` of the solver, `Float64` for double-precision data.

## Practical Examples

### Example 1

Double-precision data give a `Float64` plan.

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
  const mpc::Plan p = mpc::build(integrator(), weights1(), limits(), horizon(4));
  out.writeln("Float64:", p.dtype() == pluss::ScalarType::Float64);
}
```

Output

```text
Float64: true
```

### Example 2

The dtype does not depend on the horizon.

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
  for (const std::int64_t n : {2, 10}) out.writeln("horizon", n, " Float64:", mpc::build(cart(), weights2(), limits(), horizon(n)).dtype() == pluss::ScalarType::Float64);
}
```

Output

```text
horizon 2  Float64: true
horizon 10  Float64: true
```

### Example 3

Print the dtype name.

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
  const mpc::Plan p = mpc::build(integrator(), weights1(), limits(), horizon(4));
  out.writeln("dtype:", dtype_of(p.dtype()).name);
}
```

Output

```text
dtype: float64
```

## See Also

- [`platform`](platform.md): the device

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="horizon.html">
    <span class="label">Next</span>
    <span class="title">horizon</span>
  </a>
</div>
<!-- apiref:nav:end -->
