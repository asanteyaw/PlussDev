<!-- generated from butter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::butter`

Designs a Butterworth filter, with the flattest possible passband.

## Signature

```cpp
lti::PoleZero signal::butter(
    std::int64_t order,
    double cutoff,
    const signal::IirOptions& options = {});

lti::PoleZero signal::butter(
    std::int64_t order,
    std::pair<double, double> cutoff,
    const signal::IirOptions& options);
```

## What It Does

Returns the poles, zeros and gain of a Butterworth filter. Its magnitude is monotone and maximally flat at zero frequency, and at the cutoff it is exactly `1/sqrt(2)`, that is -3.01 dB, for every order. Raising the order sharpens the transition without adding ripple.

## Parameters

`order`
: Positive prototype order. Band-pass and band-stop designs double it.

`cutoff`
: A finite positive scalar for `Low` and `High`, or an increasing positive pair for `Pass` and `Stop`. In a digital design it is in the units of `sample_rate` and must be below the Nyquist frequency (half the sample rate).

`options`
: An `IirOptions`: band, sample rate, dtype and platform.

## Returns

An `lti::PoleZero`: discrete when a sample rate is given, continuous otherwise. Its poles lie inside the unit circle (digital) or in the left half-plane (analog).

## Errors

A nonpositive order, a nonfinite or nonpositive cutoff, a digital cutoff at or above the Nyquist frequency, an unordered cutoff pair and a cutoff form that does not match the band throw `pluss::Error`.

## Interpretation and Use

Choose Butterworth when ripple is unacceptable in either band. The gain at the cutoff is -3.01 dB at every order (first and second examples), and each added pair of poles steepens the stopband by about 14 dB at twice the cutoff: -14.2 dB at order 2, -28.0 dB at order 4, -41.9 dB at order 6. Apply the design with `lti::to_sos` and `signal::sos`.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The gain across the passband and stopband of a fourth-order low-pass, and its monotone passband.

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
  const lti::PoleZero lowpass = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  for (const double f : {0.0, 50.0, 100.0, 200.0, 400.0}) out.writeln("gain at", f, "Hz =", gain_db(lowpass, f), "dB");
  out.writeln("the passband is monotone: gain at 25 Hz exceeds gain at 75 Hz:", gain_db(lowpass, 25.0) > gain_db(lowpass, 75.0));
}
```

Output

```text
gain at 0.000 Hz = 0.000 dB
gain at 50.000 Hz = -0.014 dB
gain at 100.000 Hz = -3.010 dB
gain at 200.000 Hz = -27.966 dB
gain at 400.000 Hz = -78.116 dB
the passband is monotone: gain at 25 Hz exceeds gain at 75 Hz: true
```
### Example 2

Order 2, 4 and 6: the same gain at the cutoff and a steeper stopband.

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
  for (const std::int64_t order : {2, 4, 6}) {
    const lti::PoleZero design = sig::butter(order, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
    out.writeln("order", order, ": poles", design.poles().numel(), " gain at the cutoff", gain_db(design, 100.0), "dB, at 200 Hz", gain_db(design, 200.0), "dB");
  }
}
```

Output

```text
order 2 : poles 2  gain at the cutoff -3.010 dB, at 200 Hz -14.150 dB
order 4 : poles 4  gain at the cutoff -3.010 dB, at 200 Hz -27.966 dB
order 6 : poles 6  gain at the cutoff -3.010 dB, at 200 Hz -41.938 dB
```
### Example 3

Every refused argument.

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
  refused(out, "order zero", [] { sig::butter(0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a cutoff at the Nyquist frequency", [] { sig::butter(3, 500.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a negative cutoff", [] { sig::butter(3, -10.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a scalar cutoff for a pass band", [] { sig::butter(3, 100.0, {.band = sig::Band::Pass, .sample_rate = 1000.0}); });
  refused(out, "an unordered cutoff pair", [] { sig::butter(3, {200.0, 100.0}, {.band = sig::Band::Pass, .sample_rate = 1000.0}); });
}
```

Output

```text
order zero : eng::signal::butter: order must be positive
a cutoff at the Nyquist frequency : eng::signal::butter: a digital cutoff must be below the Nyquist frequency
a negative cutoff : eng::signal::butter: cutoff must be finite and positive
a scalar cutoff for a pass band : eng::signal::butter: a scalar cutoff requires Band::Low or Band::High
an unordered cutoff pair : eng::signal::butter: cutoffs must be finite, positive, and increasing
```
## See Also

See [IIR design](../design.md) for prototype tradeoffs and digital-versus-analog frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IirOptions.html">
    <span class="label">Previous</span>
    <span class="title">IirOptions</span>
  </a>
  <a class="next-link" href="cheby1.html">
    <span class="label">Next</span>
    <span class="title">cheby1</span>
  </a>
</div>
<!-- apiref:nav:end -->
