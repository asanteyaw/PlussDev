<!-- generated from zero_phase.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::zero_phase`

Filters forward and backward so that the output has no delay.

## Signature

```cpp
pluss::Series signal::zero_phase(
    const pluss::Series& input,
    const pluss::Series& numerator,
    const pluss::Series& denominator,
    const signal::PhaseOpts& options = {});

pluss::Series signal::zero_phase(
    const pluss::Series& input,
    const pluss::Series& sections,
    const signal::PhaseOpts& options = {});
```

## What It Does

Runs the filter over the signal, reverses the result, runs it again and reverses back. The delay of the first pass is undone by the second, so features stay at the sample where they occurred. The signal is first extended at both ends by the pad mode so that the start-up transients fall outside the kept samples. There is a polynomial overload and a sections overload.

## Parameters

`input`
: A floating or complex `Series`.

`numerator`, `denominator` or `sections`
: The filter as polynomials or as `[section, 6]` biquads.

`options`
: A `PhaseOpts`: the axis, the extension length and the pad mode.

## Returns

A `Series` with the shape of `input`.

## Errors

An input no longer than the extension, a negative edge other than `-1`, an invalid axis and malformed coefficients or sections throw `pluss::Error`.

## Interpretation and Use

A causal filter delays a pulse; `zero_phase` does not (the first example: the causal filter moves the peak by one sample, `zero_phase` keeps it). The price is non-causality: the output at a sample depends on later samples, so use it offline, not in a stream.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A pulse through a causal and a zero-phase filter: where does the peak land?

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series pulse = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const auto peak_of = [](const pluss::Series &s) {
    const auto v = s.items<double>();
    return std::max_element(v.begin(), v.end()) - v.begin();
  };
  const pluss::Series causal = sig::filter(pulse, b, a).values;
  const pluss::Series centred = sig::zero_phase(pulse, b, a, {.dim = 0, .edge = 2});
  out.writeln("the pulse peaks at sample", peak_of(pulse));
  out.writeln("the causal filter moves the peak to sample", peak_of(causal), "(it lags)");
  out.writeln("zero_phase keeps the peak at sample", peak_of(centred));
  out.writeln("zero-phase output =", centred.items<double>());
}
```

Output

```text
the pulse peaks at sample 7
the causal filter moves the peak to sample 8 (it lags)
zero_phase keeps the peak at sample 7
zero-phase output = [0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.6250, 2.5000, 3.7500, 2.5000, 0.6250, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000]
```
### Example 2

The sections overload agrees with the polynomial overload.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series sections = pluss::make_series({0.25, 0.5, 0.25, 1.0, 0.0, 0.0}).reshape({1, 6});
  const pluss::Series pulse = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const auto by_sections = sig::zero_phase(pulse, sections, {.dim = 0, .edge = 2}).items<double>();
  const auto by_polynomial = sig::zero_phase(pulse, b, a, {.dim = 0, .edge = 2}).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < by_sections.size(); ++i) worst = std::max(worst, std::abs(by_sections[i] - by_polynomial[i]));
  out.writeln("sections overload against polynomial overload, largest difference =", worst);
}
```

Output

```text
sections overload against polynomial overload, largest difference = 0.0000
```
### Example 3

Every refused input.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  refused(out, "an input shorter than the edge extension", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0}), b, a, {.dim = 0, .edge = 5}); });
  refused(out, "a negative edge other than -1", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), b, a, {.dim = 0, .edge = -2}); });
  refused(out, "an axis out of range", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), b, a, {.dim = 4}); });
}
```

Output

```text
an input shorter than the edge extension : eng::signal::zero_phase: sample axis must be longer than the edge extension
a negative edge other than -1 : eng::signal::zero_phase: edge must be -1 or nonnegative
an axis out of range : eng::signal::zero_phase: dim out of range
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sos.html">
    <span class="label">Previous</span>
    <span class="title">sos</span>
  </a>
  <a class="next-link" href="freqz.html">
    <span class="label">Next</span>
    <span class="title">freqz</span>
  </a>
</div>
<!-- apiref:nav:end -->
