<!-- generated from PoleZero.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::PoleZero`

## Signature

```cpp
const pluss::Series& lti::PoleZero::zeros() const noexcept;
const pluss::Series& lti::PoleZero::poles() const noexcept;
const pluss::Series& lti::PoleZero::gain() const noexcept;

double lti::PoleZero::sample_time() const noexcept;
bool lti::PoleZero::is_cont() const noexcept;
bool lti::PoleZero::is_disc() const noexcept;
bool lti::PoleZero::is_stable() const;
```

## What It Represents

An immutable proper transfer-system representation. Zeros and poles are stored in a common complex precision, gain is scalar, and timebase determines the stability region and frequency mapping.

## Methods

`zeros()`, `poles()`, `gain()`
: Return const references to root vectors and scalar gain.

`sample_time()`
: Zero for continuous time or positive for discrete time.

`is_cont()`, `is_disc()`
: Report the timebase.

`is_stable()`
: Tests negative pole real parts in continuous time or pole magnitudes below one in discrete time.

## Returns

Accessors return stored payloads and metadata. Construct through `lti::zpk` or a signal-design factory.

## Errors

Accessors are nonthrowing except stability evaluation may execute `Series` reductions on the stored platform.

## Changelog

- **0.1** — Initial zero-pole-gain system contract.

## Practical Examples

These verified programs cover the normal contract, a materially different mode or analysis, and validation behavior.

### Example 1

Inspect stability, response, and second-order-section realization.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>
#include <iostream>

namespace lti = pluss::ma::eng::lti;
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

namespace lti = pluss::ma::eng::lti;
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

namespace lti = pluss::ma::eng::lti;
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

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="zpk.html">
    <span class="label">Next</span>
    <span class="title">zpk</span>
  </a>
</div>
<!-- apiref:nav:end -->
