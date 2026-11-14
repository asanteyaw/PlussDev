<!-- generated from zpk.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::zpk`

## Signature

```cpp
lti::PoleZero lti::zpk(
    const pluss::Series& zeros,
    const pluss::Series& poles,
    const pluss::Series& gain,
    double sample_time = 0.0);
```

## What It Does

Constructs a proper zero-pole-gain system, promotes roots and gain to one complex precision, and records a continuous or discrete timebase.

## Parameters

`zeros`
: Finite rank-one floating or complex transmission zeros.

`poles`
: Finite rank-one floating or complex poles. A proper system has at least as many poles as zeros.

`gain`
: One finite floating or complex value.

`sample_time`
: Zero for continuous time or finite positive sampling interval for discrete time.

## Returns

A `PoleZero` on the common input platform with normalized complex root storage and scalar gain.

## Errors

Rejects undefined, nonvector, nonfinite, integer, cross-platform, or improper root data; nonscalar gain; and invalid sample time.

## Practical Examples

These verified programs cover the normal contract, a materially different mode or analysis, and validation behavior.

### Example 1

Inspect stability, response, and second-order-section realization.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::PoleZero system = lti::zpk(
      pluss::make_series({0.0}), pluss::make_series({0.5}),
      pluss::make_series(0.5), 1.0);
  const pluss::Series response =
      lti::freqresp(system, pluss::make_series({0.0, 3.141592653589793}));

  std::cout << std::boolalpha << "discrete = " << system.is_disc()
            << ", stable = " << system.is_stable() << "\n";
  out.writeln("zeros =", system.zeros().items<std::complex<double>>());
  out.writeln("poles =", system.poles().items<std::complex<double>>());
  out.writeln("response =", response.items<std::complex<double>>());
  out.writeln("sos =", lti::to_sos(system).flatten().items<double>());
}
```

Output

```text
discrete = true, stable = true
zeros = [(0,0)]
poles = [(0.5,0)]
response = [(1,0), (0.333333,-1.36072e-17)]
sos = [0.5000, 0.0000, 0.0000, 1.0000, -0.5000, 0.0000]
```
### Example 2

Compare frequency response across equivalent system representations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  const lti::StateSpace state_system = lti::state(
      pluss::make_series({-1.0}, {1, 1}), pluss::make_series({1.0}, {1, 1}),
      pluss::make_series({1.0}, {1, 1}), pluss::make_series({0.0}, {1, 1}));
  const lti::PoleZero root_system = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({-1.0}),
      pluss::make_series(1.0));
  const pluss::Series omega = pluss::make_series({0.0, 1.0, 2.0});

  out.writeln("state response =",
              lti::freqresp(state_system, omega).flatten().items<std::complex<double>>());
  out.writeln("root response =",
              lti::freqresp(root_system, omega).items<std::complex<double>>());
}
```

Output

```text
state response = [(1,0), (0.5,-0.5), (0.2,-0.4)]
root response = [(1,0), (0.5,-0.5), (0.2,-0.4)]
```
### Example 3

Exercise frequency-grid and realization validation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const lti::PoleZero system = lti::zpk(
      pluss::series::empty({0}, pluss::ScalarType::Float64),
      pluss::make_series({-1.0}),
      pluss::make_series(1.0));
  try {
    static_cast<void>(lti::freqresp(system, pluss::make_series({0, 1})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("integer grid rejected =", message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(lti::to_sos(system));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("continuous SOS rejected =", message.substr(0, message.find('\n')));
  }
}
```

Output

```text
integer grid rejected = eng::lti::freqresp: omega must have a real floating dtype
continuous SOS rejected = eng::lti::to_sos: only discrete systems can be realized by the delay-form SOS filter
```
## See Also

See [Pole-zero and response](../response.md) for timebase, stability, and angular-frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PoleZero.html">
    <span class="label">Previous</span>
    <span class="title">PoleZero</span>
  </a>
  <a class="next-link" href="to_sos.html">
    <span class="label">Next</span>
    <span class="title">to_sos</span>
  </a>
</div>
<!-- apiref:nav:end -->
