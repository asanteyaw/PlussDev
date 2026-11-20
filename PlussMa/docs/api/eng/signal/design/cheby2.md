<!-- generated from cheby2.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::cheby2`

Designs a Chebyshev type II filter: a flat passband and an equal-ripple stopband.

## Signature

```cpp
lti::PoleZero signal::cheby2(
    std::int64_t order,
    double attenuation,
    double cutoff,
    const signal::IirOptions& options = {});

lti::PoleZero signal::cheby2(
    std::int64_t order,
    double attenuation,
    std::pair<double, double> cutoff,
    const signal::IirOptions& options);
```

## What It Does

Returns the poles, zeros and gain of a Chebyshev type II (inverse Chebyshev) filter. The passband is monotone and the stopband gain never rises above `-attenuation` dB. Unlike the other designs, the cutoff is the edge of the stopband: the gain there is exactly `-attenuation` dB, not -3 dB.

## Parameters

`order`
: Positive prototype order. Band-pass and band-stop designs double it.

`attenuation`
: Minimum stopband attenuation in decibels, finite and positive.

`cutoff`
: A finite positive scalar for `Low` and `High`, or an increasing positive pair for `Pass` and `Stop`. For this design it is the stopband edge. In a digital design it is in the units of `sample_rate` and must be below the Nyquist frequency (half the sample rate).

`options`
: An `IirOptions`: band, sample rate, dtype and platform.

## Returns

An `lti::PoleZero` with stable poles.

## Errors

A nonpositive order or attenuation, a bad cutoff and a cutoff form that does not match the band throw `pluss::Error`.

## Interpretation and Use

Because the cutoff is where the stopband starts, the -3 dB point lies below it (about 50 Hz for the design in the examples). The first example scans from the stopband edge to the Nyquist frequency and finds the highest gain at -40.000 dB. The second shows the trade against Butterworth: far better rejection just above the edge, but a stopband that stays near -40 dB where a Butterworth filter keeps falling (at 300 Hz Butterworth is lower).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The stopband bound and the monotone passband of a fourth-order filter with 40 dB of attenuation.

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
  const lti::PoleZero design = sig::cheby2(4, 40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  double highest_stop = -1e9;
  for (double f = 100.0; f <= 499.0; f += 1.0) highest_stop = std::max(highest_stop, gain_db(design, f));
  out.writeln("highest gain from 100 Hz to the Nyquist frequency =", highest_stop, "dB for an attenuation of 40 dB");
  for (const double f : {0.0, 25.0, 50.0, 75.0}) out.writeln("passband gain at", f, "Hz =", gain_db(design, f), "dB");
}
```

Output

```text
highest gain from 100 Hz to the Nyquist frequency = -40.000 dB for an attenuation of 40 dB
passband gain at 0.000 Hz = -0.000 dB
passband gain at 25.000 Hz = -0.009 dB
passband gain at 50.000 Hz = -2.651 dB
passband gain at 75.000 Hz = -17.672 dB
```
### Example 2

The same cutoff against a Butterworth filter: where each is lower.

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
  const lti::PoleZero inverse = sig::cheby2(4, 40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero butterworth = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  for (const double f : {50.0, 100.0, 150.0, 300.0}) out.writeln("at", f, "Hz: Chebyshev II", gain_db(inverse, f), "dB, Butterworth", gain_db(butterworth, f), "dB");
}
```

Output

```text
at 50.000 Hz: Chebyshev II -2.651 dB, Butterworth -0.014 dB
at 100.000 Hz: Chebyshev II -40.000 dB, Butterworth -3.010 dB
at 150.000 Hz: Chebyshev II -40.628 dB, Butterworth -15.748 dB
at 300.000 Hz: Chebyshev II -44.746 dB, Butterworth -50.157 dB
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
  refused(out, "zero attenuation", [] { sig::cheby2(3, 0.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "negative attenuation", [] { sig::cheby2(3, -40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "order zero", [] { sig::cheby2(0, 40.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a cutoff at the Nyquist frequency", [] { sig::cheby2(3, 40.0, 500.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
}
```

Output

```text
zero attenuation : eng::signal::cheby2: attenuation must be finite and positive
negative attenuation : eng::signal::cheby2: attenuation must be finite and positive
order zero : eng::signal::cheby2: order must be positive
a cutoff at the Nyquist frequency : eng::signal::cheby2: a digital cutoff must be below the Nyquist frequency
```
## See Also

See [IIR design](../design.md) for prototype tradeoffs and digital-versus-analog frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cheby1.html">
    <span class="label">Previous</span>
    <span class="title">cheby1</span>
  </a>
</div>
<!-- apiref:nav:end -->
