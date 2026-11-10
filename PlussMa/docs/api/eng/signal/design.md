<!-- generated from design.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# IIR design

The IIR design family converts an order and frequency specification into an `lti::PoleZero` system. Pole-zero output is the stable interchange representation: inspect stability and continuity directly, convert to second-order sections for numerical filtering, or evaluate the response through the LTI family.

## Digital And Analog Interpretation

When `IirOptions::sample_rate` has a value, cutoff frequencies use the same ordinary-frequency units as that rate. A sample rate of `1000.0` therefore accepts a cutoff of `100.0` and rejects values at or beyond the `500.0` Nyquist boundary. When `sample_rate` is `std::nullopt`, the design is continuous and cutoffs are angular frequencies.

`Band::Low` and `Band::High` take a scalar cutoff. `Band::Pass` and `Band::Stop` take an ordered cutoff pair. The API keeps these overloads separate so a mismatched specification fails immediately rather than being reinterpreted.

## Prototype Choice

Butterworth designs are maximally flat in the passband. Chebyshev type I accepts passband ripple to sharpen transition. Chebyshev type II keeps the passband monotonic and places equiripple behavior in the stopband, controlled through minimum attenuation. Order, ripple, and attenuation are design inputs, not post-processing hints.

## Practical Examples

The examples compare all three prototypes, exercise low-pass, band-pass, band-stop, digital, and analog designs, and show validation at Nyquist and at mismatched scalar/band specifications.

### Example 1

Design three fourth-order digital low-pass systems, inspect stability, and convert the Butterworth result to second-order sections.

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

Compare band-pass and band-stop responses and construct a continuous-time analog Butterworth system.

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

Demonstrate rejected Nyquist cutoffs, invalid Chebyshev ripple, and a scalar cutoff paired with a two-edge band kind.

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
## Related Pages

Use [Filtering and response](filtering.md) for sample-domain execution and `eng::lti` for system conversion, state-space analysis, and general frequency response.

```{toctree}
:maxdepth: 1
:hidden:

design/Band
design/IirOptions
design/butter
design/cheby1
design/cheby2
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

design/Band
design/IirOptions
design/butter
design/cheby1
design/cheby2
```
<!-- apiref:toctree:end -->
