<!-- generated from platform.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0521762b129d -->

# `control::mpc::Plan::platform`

[Back to `control::mpc::Plan`](../Plan.md)

The device the plan's solver lives on.

## Signature

```cpp
pluss::Platform control::mpc::Plan::platform() const noexcept;
```

## What It Does

Returns the platform (CPU or an accelerator) on which the factorised problem is stored and solved. Data passed to a solve is moved there as needed.

## Returns

The `Platform` of the solver.

## Practical Examples

### Example 1

A plan built from CPU data stays on the CPU.

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
  out.writeln("on the CPU:", p.platform() == pluss::Platform::CPU);
}
```

Output

```text
on the CPU: true
```

### Example 2

The platform does not depend on the horizon.

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
  for (const std::int64_t n : {2, 10}) out.writeln("horizon", n, " on the CPU:", mpc::build(cart(), weights2(), limits(), horizon(n)).platform() == pluss::Platform::CPU);
}
```

Output

```text
horizon 2  on the CPU: true
horizon 10  on the CPU: true
```

### Example 3

The platform and dtype together describe where and how it solves.

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
  const mpc::Plan p = mpc::build(cart(), weights2(), limits(), horizon(4));
  out.writeln("platform is CPU:", p.platform() == pluss::Platform::CPU, " dtype:", dtype_of(p.dtype()).name);
}
```

Output

```text
platform is CPU: true  dtype: float64
```

## See Also

- [`dtype`](dtype.md): the floating dtype

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inputs.html">
    <span class="label">Previous</span>
    <span class="title">inputs</span>
  </a>
  <a class="next-link" href="states.html">
    <span class="label">Next</span>
    <span class="title">states</span>
  </a>
</div>
<!-- apiref:nav:end -->
