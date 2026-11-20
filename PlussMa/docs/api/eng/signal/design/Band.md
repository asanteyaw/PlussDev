<!-- generated from Band.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::Band`

Which part of the spectrum a designed filter passes.

## Signature

```cpp
enum class signal::Band { Low, High, Pass, Stop };
```

## What It Does

The prototypes `butter`, `cheby1` and `cheby2` are low-pass filters. `Band` selects the transformation applied to the prototype: `Low` keeps it, `High` mirrors it to pass the highs, `Pass` keeps a band between two cutoffs and `Stop` removes one. The transformations that need two edges double the order.

## Values

| Value | Passes | Cutoff argument | Poles of an order-`n` design |
|---|---|---|---|
| `Low` | Below the cutoff. | One scalar. | `n` |
| `High` | Above the cutoff. | One scalar. | `n` |
| `Pass` | Between two cutoffs. | An increasing pair. | `2n` |
| `Stop` | Outside two cutoffs. | An increasing pair. | `2n` |

## Errors

A scalar cutoff with `Pass` or `Stop`, a pair with `Low` or `High`, and a pair with an edge at or above the Nyquist frequency throw `pluss::Error`.

## Interpretation and Use

Pick the band by what must survive: a low-pass removes noise above a frequency, a high-pass removes drift below one, a band-pass isolates a signal and a band-stop removes an interferer such as mains hum. The examples show the gain at the band edges and in the rejected region for each.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Low-pass and high-pass at the same cutoff: opposite behaviour, the same gain at the edge.

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
  const std::pair<const char *, sig::Band> bands[] = {{"Low ", sig::Band::Low}, {"High", sig::Band::High}};
  for (const auto &[label, band] : bands) {
    const lti::PoleZero design = sig::butter(3, 100.0, {.band = band, .sample_rate = 1000.0});
    out.writeln(label, ": poles", design.poles().numel(), " gain at 10 Hz", gain_db(design, 10.0), "dB, at 100 Hz", gain_db(design, 100.0), "dB, at 400 Hz", gain_db(design, 400.0), "dB");
  }
}
```

Output

```text
Low  : poles 3  gain at 10 Hz -0.000 dB, at 100 Hz -3.010 dB, at 400 Hz -58.587 dB
High : poles 3  gain at 10 Hz -60.869 dB, at 100 Hz -3.010 dB, at 400 Hz -0.000 dB
```
### Example 2

Band-pass and band-stop between 100 and 200 Hz: complementary gains and doubled order.

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
  const std::pair<const char *, sig::Band> bands[] = {{"Pass", sig::Band::Pass}, {"Stop", sig::Band::Stop}};
  for (const auto &[label, band] : bands) {
    const lti::PoleZero design = sig::butter(3, {100.0, 200.0}, {.band = band, .sample_rate = 1000.0});
    out.writeln(label, ": poles", design.poles().numel(), " (a third-order prototype doubles); gain at 50 Hz", gain_db(design, 50.0), "dB, at 150 Hz", gain_db(design, 150.0), "dB, at 400 Hz", gain_db(design, 400.0), "dB");
  }
}
```

Output

```text
Pass : poles 6  (a third-order prototype doubles); gain at 50 Hz -31.246 dB, at 150 Hz -0.000 dB, at 400 Hz -52.407 dB
Stop : poles 6  (a third-order prototype doubles); gain at 50 Hz -0.003 dB, at 150 Hz -56.342 dB, at 400 Hz -0.000 dB
```
### Example 3

Every mismatch between a band and its cutoff.

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
  refused(out, "a scalar cutoff for a pass band", [] { sig::butter(3, 100.0, {.band = sig::Band::Pass, .sample_rate = 1000.0}); });
  refused(out, "a scalar cutoff for a stop band", [] { sig::butter(3, 100.0, {.band = sig::Band::Stop, .sample_rate = 1000.0}); });
  refused(out, "a cutoff pair for a low-pass", [] { sig::butter(3, {100.0, 200.0}, {.band = sig::Band::Low, .sample_rate = 1000.0}); });
  refused(out, "a pair with an edge above the Nyquist frequency", [] { sig::butter(3, {100.0, 600.0}, {.band = sig::Band::Pass, .sample_rate = 1000.0}); });
}
```

Output

```text
a scalar cutoff for a pass band : eng::signal::butter: a scalar cutoff requires Band::Low or Band::High
a scalar cutoff for a stop band : eng::signal::butter: a scalar cutoff requires Band::Low or Band::High
a cutoff pair for a low-pass : eng::signal::butter: two cutoffs require Band::Pass or Band::Stop
a pair with an edge above the Nyquist frequency : eng::signal::butter: digital cutoffs must be below the Nyquist frequency
```
## See Also

See [IIR design](../design.md) for prototype tradeoffs and digital-versus-analog frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="IirOptions.html">
    <span class="label">Next</span>
    <span class="title">IirOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
