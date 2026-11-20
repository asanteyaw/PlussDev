<!-- generated from cheby1.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::cheby1`

Designs a Chebyshev type I filter: equal ripple in the passband, a steeper transition.

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

Returns the poles, zeros and gain of a Chebyshev type I filter. The passband gain oscillates between 0 and `-ripple` dB and the cutoff is the edge of the passband, where the gain is exactly `-ripple` dB. In exchange for the passband ripple the stopband falls faster than a Butterworth filter of the same order.

## Parameters

`order`
: Positive prototype order. Band-pass and band-stop designs double it.

`ripple`
: Peak-to-peak passband ripple in decibels, finite and positive.

`cutoff`
: A finite positive scalar for `Low` and `High`, or an increasing positive pair for `Pass` and `Stop`. In a digital design it is in the units of `sample_rate` and must be below the Nyquist frequency (half the sample rate).

`options`
: An `IirOptions`: band, sample rate, dtype and platform.

## Returns

An `lti::PoleZero` with stable poles.

## Errors

A nonpositive order or ripple, a bad cutoff and a cutoff form that does not match the band throw `pluss::Error`.

## Interpretation and Use

The first example sweeps the passband and finds the gain between -1.000 and 0.000 dB for a ripple of 1 dB. The second compares it with Butterworth of the same order: at 200 Hz it is 10 dB lower (-38.3 dB against -28.0 dB). Choose it when a sharp transition matters more than a flat passband.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The passband ripple of a fourth-order filter with a ripple of 1 dB.

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
  const lti::PoleZero design = sig::cheby1(4, 1.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  double highest = -1e9, lowest = 1e9;
  for (double f = 1.0; f <= 100.0; f += 1.0) {
    const double g = gain_db(design, f);
    highest = std::max(highest, g);
    lowest = std::min(lowest, g);
  }
  out.writeln("passband (1 to 100 Hz) spans", lowest, "to", highest, "dB for a ripple of 1 dB");
  out.writeln("gain at 200 Hz =", gain_db(design, 200.0), "dB");
}
```

Output

```text
passband (1 to 100 Hz) spans -1.000 to -0.000 dB for a ripple of 1 dB
gain at 200 Hz = -38.269 dB
```
### Example 2

The stopband against a Butterworth filter of the same order and cutoff.

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
  const lti::PoleZero chebyshev = sig::cheby1(4, 1.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const lti::PoleZero butterworth = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  for (const double f : {150.0, 200.0, 300.0}) out.writeln("at", f, "Hz: Chebyshev I", gain_db(chebyshev, f), "dB, Butterworth", gain_db(butterworth, f), "dB");
}
```

Output

```text
at 150.000 Hz: Chebyshev I -23.607 dB, Butterworth -15.748 dB
at 200.000 Hz: Chebyshev I -38.269 dB, Butterworth -27.966 dB
at 300.000 Hz: Chebyshev I -61.856 dB, Butterworth -50.157 dB
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
  refused(out, "zero ripple", [] { sig::cheby1(3, 0.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "negative ripple", [] { sig::cheby1(3, -1.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "order zero", [] { sig::cheby1(0, 1.0, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a cutoff at the Nyquist frequency", [] { sig::cheby1(3, 1.0, 500.0, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
}
```

Output

```text
zero ripple : eng::signal::cheby1: ripple must be finite and positive
negative ripple : eng::signal::cheby1: ripple must be finite and positive
order zero : eng::signal::cheby1: order must be positive
a cutoff at the Nyquist frequency : eng::signal::cheby1: a digital cutoff must be below the Nyquist frequency
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
