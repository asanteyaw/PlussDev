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
| `platform` | `Platform::CPU` | Storage and execution platform of the resulting pole-zero payload. On the MPS the dtype must be `Float32`, because the MPS has no 64-bit floats. |

## Returns

Pass by const reference to a design factory; the result is an `lti::PoleZero` carrying the selected continuous/digital interpretation.

## Errors

Rejects nonpositive or nonfinite sample rates, a dtype other than `Float32` or `Float64`, a `Float64` design on the MPS (the MPS has no 64-bit floats; use `Float32`), or a band incompatible with the chosen cutoff overload.

## Interpretation and Use

With a `sample_rate` the cutoffs are in hertz (or any unit consistent with the rate) and the result is discrete with sample time `1 / sample_rate`; with `std::nullopt` the cutoffs are angular frequencies in radians per second and the result is continuous. The default rate of 2 makes a cutoff of 1 the Nyquist frequency, the usual normalised convention. Set `dtype` to `Float32` to build the design in single precision.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A digital design with a sample rate against an analog one without, and the default rate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace sig = pluss::subsp::eng::signal;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

// Gain in decibels of a design at a physical frequency in hertz.
static double gain_db(const lti::PoleZero &design, double hertz) {
  const pluss::Series omega = pluss::make_series({2.0 * std::acos(-1.0) * hertz});
  return 20.0 * std::log10(lti::freqresp(design, omega).abs().items<double>()[0]);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const lti::PoleZero digital = sig::butter(2, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero analog = sig::butter(2, 100.0, {.band = sig::Band::Low, .sample_rate = std::nullopt});
  out.writeln("with a sample rate: discrete =", digital.is_disc(), " sample time =", digital.sample_time(), " stable =", digital.is_stable());
  out.writeln("without one: continuous =", analog.is_cont(), " stable =", analog.is_stable());
  const sig::IirOptions defaults;
  out.writeln("the default sample rate is", *defaults.sample_rate, "(a cutoff of 0.5 is then half the Nyquist frequency)");
}
```

Output

```text
with a sample rate: discrete = true  sample time = 0.001  stable = true
without one: continuous = true  stable = true
the default sample rate is 2.000 (a cutoff of 0.5 is then half the Nyquist frequency)
```
### Example 2

Float32 against Float64 poles: the dtype of the design and the same gain at the cutoff.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace sig = pluss::subsp::eng::signal;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

// Gain in decibels of a design at a physical frequency in hertz.
static double gain_db(const lti::PoleZero &design, double hertz) {
  const pluss::Series omega = pluss::make_series({2.0 * std::acos(-1.0) * hertz});
  return 20.0 * std::log10(lti::freqresp(design, omega).abs().items<double>()[0]);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const lti::PoleZero wide = sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero narrow = sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0, .dtype = pluss::ScalarType::Float32});
  out.writeln("poles dtype", wide.poles().scalar_type(), "against", narrow.poles().scalar_type());
  out.writeln("gain at the cutoff", gain_db(wide, 100.0), "dB against", gain_db(narrow, 100.0), "dB");
}
```

Output

```text
poles dtype complex128 against complex64
gain at the cutoff -3.010 dB against -3.010 dB
```
### Example 3

Every refused option.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace sig = pluss::subsp::eng::signal;
namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

// Gain in decibels of a design at a physical frequency in hertz.
static double gain_db(const lti::PoleZero &design, double hertz) {
  const pluss::Series omega = pluss::make_series({2.0 * std::acos(-1.0) * hertz});
  return 20.0 * std::log10(lti::freqresp(design, omega).abs().items<double>()[0]);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  refused(out, "a negative sample rate", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = -1000.0}); });
  refused(out, "a zero sample rate", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 0.0}); });
  refused(out, "a non-floating dtype", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0, .dtype = pluss::ScalarType::Int64}); });
  refused(out, "a float64 design on the MPS", [] { sig::butter(3, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0, .platform = pluss::Platform::MPS}); });
}
```

Output

```text
a negative sample rate : eng::signal::butter: sample_rate must be finite and positive
a zero sample rate : eng::signal::butter: sample_rate must be finite and positive
a non-floating dtype : eng::signal::butter: dtype must be Float32 or Float64
a float64 design on the MPS : eng::signal::butter: float64 is unsupported on the MPS; set dtype to Float32 to design on it
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
