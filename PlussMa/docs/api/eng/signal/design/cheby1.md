<!-- generated from cheby1.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::cheby1`

## Signature

```cpp
lti::PoleZero signal::cheby1(
    std::int64_t order,
    double ripple,
    double cutoff,
    const signal::IirOptions& options = {});

lti::PoleZero signal::cheby1(
    std::int64_t order,
    double ripple,
    std::pair<double, double> cutoff,
    const signal::IirOptions& options);
```

## What It Does

Designs a Chebyshev type-I system. Positive passband ripple permits a steeper transition than a Butterworth design of the same order.

## Parameters

`order`
: Positive prototype order. Band-pass and band-stop transformations produce the corresponding doubled system order.

`ripple`
: Positive passband ripple in decibels. Larger values permit greater passband variation.

`cutoff`
: A finite positive scalar for low/high designs or an increasing positive pair for pass/stop designs. Digital cutoffs must remain below Nyquist.

`options`
: Band, sample-rate interpretation, dtype, and platform policy.

## Returns

An `lti::PoleZero` system. Digital results are discrete and should have poles inside the unit circle; analog results are continuous and should have poles in the left half-plane.

## Errors

Rejects nonpositive order, invalid or unordered cutoffs, digital cutoffs at or above Nyquist, invalid prototype ripple/attenuation, and cutoff-form/band mismatches.

## Practical Examples

These verified programs show the normal contract, an important alternative or stateful workflow, and validation behavior.

### Example 1

Compare the three prototype families and inspect a second-order-section conversion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;

  const sig::IirOptions options{
      .band = sig::Band::Low,
      .sample_rate = 1000.0,
  };
  const lti::PoleZero butter = sig::butter(4, 100.0, options);
  const lti::PoleZero cheby1 = sig::cheby1(4, 1.0, 100.0, options);
  const lti::PoleZero cheby2 = sig::cheby2(4, 40.0, 100.0, options);

  out.writeln("Butterworth poles =", butter.poles().numel());
  out.writeln("Butterworth stable =", butter.is_stable());
  out.writeln("Chebyshev I stable =", cheby1.is_stable());
  out.writeln("Chebyshev II stable =", cheby2.is_stable());
  out.writeln("SOS shape =", lti::to_sos(butter).sizes());
}
```

Output

```text
Butterworth poles = 4
Butterworth stable = true
Chebyshev I stable = true
Chebyshev II stable = true
SOS shape = [2, 6]
```
### Example 2

Exercise band transformations and analog-versus-digital interpretation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <optional>

namespace lti = pluss::subsp::eng::lti;
namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const lti::PoleZero pass = sig::butter(
      3, {100.0, 200.0},
      {.band = sig::Band::Pass, .sample_rate = 1000.0});
  const lti::PoleZero stop = sig::butter(
      3, {100.0, 200.0},
      {.band = sig::Band::Stop, .sample_rate = 1000.0});
  const lti::PoleZero analog = sig::butter(
      3, 2.0,
      {.band = sig::Band::Low, .sample_rate = std::nullopt});
  const pluss::Series omega =
      pluss::make_series({0.0, 150.0 * 2.0 * std::acos(-1.0)});

  out.writeln("pass magnitude =", lti::freqresp(pass, omega).abs().items<double>());
  out.writeln("stop magnitude =", lti::freqresp(stop, omega).abs().items<double>());
  out.writeln("analog continuous =", analog.is_cont());
  out.writeln("analog gain =", analog.gain().real().item<double>());
}
```

Output

```text
pass magnitude = [0.0000, 1.0000]
stop magnitude = [1.0000, 0.0015]
analog continuous = true
analog gain = 8.0000
```
### Example 3

Observe validation of cutoffs, ripple, and band/cutoff shape.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const sig::IirOptions low{
      .band = sig::Band::Low,
      .sample_rate = 1000.0,
  };
  try {
    static_cast<void>(sig::butter(3, 500.0, low));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("Nyquist cutoff rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::cheby1(3, 0.0, 100.0, low));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("zero ripple rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::butter(
        3, 100.0,
        {.band = sig::Band::Pass, .sample_rate = 1000.0}));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("scalar passband rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
Nyquist cutoff rejected = eng::signal::butter: a digital cutoff must be below the Nyquist frequency
zero ripple rejected = eng::signal::cheby1: ripple must be finite and positive
scalar passband rejected = eng::signal::butter: a scalar cutoff requires Band::Low or Band::High
```
## See Also

See [IIR design](../design.md) for prototype tradeoffs and digital-versus-analog frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="butter.html">
    <span class="label">Previous</span>
    <span class="title">butter</span>
  </a>
  <a class="next-link" href="cheby2.html">
    <span class="label">Next</span>
    <span class="title">cheby2</span>
  </a>
</div>
<!-- apiref:nav:end -->
