<!-- generated from state.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::state`

## Signature

```cpp
lti::StateSpace lti::state(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& c,
    const pluss::Series& d,
    double sample_time = 0.0);
```

## What It Does

Validates and normalizes four state-space matrices into one immutable system. It broadcasts compatible leading batch axes and promotes all matrices to one floating or complex dtype.

## Parameters

`a`
: Dynamics matrix with tail `[states, states]`; it must be square.

`b`
: Input matrix with tail `[states, inputs]`.

`c`
: Output matrix with tail `[outputs, states]`.

`d`
: Direct-feedthrough matrix with tail `[outputs, inputs]`.

`sample_time`
: Zero for continuous time or a finite positive discrete sampling interval.

## Returns

A `StateSpace` owning promoted, batch-expanded matrices on their shared platform.

## Errors

Rejects undefined, nonmatrix, empty, integer, shape-incompatible, non-broadcastable, or cross-platform matrices, and negative/nonfinite sample time.

## Practical Examples

These verified programs cover the normal contract, a materially different mode or analysis, and validation behavior.

### Example 1

Construct and inspect a second-order continuous state-space model.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace system = lti::state(
      pluss::make_series({{0.0, 1.0}, {-2.0, -3.0}}),
      pluss::make_series({0.0, 1.0}, {2, 1}),
      pluss::make_series({{1.0, 0.0}}),
      pluss::make_series({0.0}, {1, 1}));

  std::cout << "states = " << system.states()
            << ", inputs = " << system.inputs()
            << ", outputs = " << system.outputs() << "\n";
  out.writeln("continuous =", system.is_cont());
  out.writeln("controllability =", lti::ctrb(system).flatten().items<double>());
  out.writeln("observability =", lti::obsv(system).flatten().items<double>());
}
```

Output

```text
states = 2, inputs = 1, outputs = 1
continuous = true
controllability = [0.0000, 1.0000, 1.0000, -3.0000]
observability = [1.0000, 0.0000, 0.0000, 1.0000]
```
### Example 2

Compare energy Gramians across continuous and discrete timebases.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::StateSpace continuous = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({2.0}, {1, 1}),
      pluss::make_series({3.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const lti::StateSpace discrete = lti::state(
      pluss::make_series({0.5}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}), 0.25);

  out.writeln("continuous reach gram =",
              lti::gram(continuous, lti::Gram::Reach).flatten().items<double>());
  out.writeln("continuous observe gram =",
              lti::gram(continuous, lti::Gram::Observe).flatten().items<double>());
  out.writeln("discrete reach gram =",
              lti::gram(discrete, lti::Gram::Reach).flatten().items<double>());
}
```

Output

```text
continuous reach gram = [2.0000]
continuous observe gram = [4.5000]
discrete reach gram = [1.3333]
```
### Example 3

Exercise matrix-shape and sample-time validation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::ma::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(lti::state(
        pluss::make_series({{1.0, 0.0}, {0.0, 1.0}}),
        pluss::make_series({1.0, 0.0}, {2, 1}),
        pluss::make_series({{1.0, 0.0}}),
        pluss::make_series({{0.0, 0.0}})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("bad feedthrough rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::state(
        pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
        pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}), -0.1));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative sample time rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
bad feedthrough rejected = eng::lti::state: D must have shape [outputs, inputs]
negative sample time rejected = eng::lti::state: sample_time must be finite and non-negative
```
## See Also

See the [State space](../state.md) family page for matrix shapes, batching, and analysis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Gram.html">
    <span class="label">Previous</span>
    <span class="title">Gram</span>
  </a>
  <a class="next-link" href="ctrb.html">
    <span class="label">Next</span>
    <span class="title">ctrb</span>
  </a>
</div>
<!-- apiref:nav:end -->
