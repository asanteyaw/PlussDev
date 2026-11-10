<!-- generated from IirOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::IirOptions`

## Signature

```cpp
signal::IirOptions options{};
```

## What It Does

Controls the band transformation, frequency interpretation, output precision, and execution platform of every IIR prototype factory.

## Fields

| Field | Default | Meaning |
|---|---:|---|
| `band` | `Band::Low` | Chooses low-pass, high-pass, band-pass, or band-stop transformation. |
| `sample_rate` | `2.0` | Present: digital design with cutoff in matching ordinary-frequency units. `std::nullopt`: continuous analog design with angular-frequency cutoffs. |
| `dtype` | `ScalarType::Float64` | Real precision used to construct poles, zeros, and gain. |
| `platform` | `Platform::CPU` | Storage and execution platform of the resulting pole-zero payload. |

## Returns

Pass by const reference to a design factory; the result is an `lti::PoleZero` carrying the selected continuous/digital interpretation.

## Errors

Rejects nonpositive/nonfinite sample rates, unsupported dtypes/platforms, or a band incompatible with the chosen cutoff overload.

## Practical Examples

These verified programs show the normal contract, an important alternative or stateful workflow, and validation behavior.

### Example 1

Compare the three prototype families and inspect a second-order-section conversion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace lti = pluss::ma::eng::lti;
namespace sig = pluss::ma::eng::signal;
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

namespace lti = pluss::ma::eng::lti;
namespace sig = pluss::ma::eng::signal;
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

namespace sig = pluss::ma::eng::signal;
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
  <a class="prev-link" href="Band.html">
    <span class="label">Previous</span>
    <span class="title">Band</span>
  </a>
  <a class="next-link" href="butter.html">
    <span class="label">Next</span>
    <span class="title">butter</span>
  </a>
</div>
<!-- apiref:nav:end -->
