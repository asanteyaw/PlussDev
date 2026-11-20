<!-- generated from PadMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::PadMode`

How `zero_phase` extends the signal at its ends.

## Signature

```cpp
signal::PadMode signal::PadMode::Odd;
signal::PadMode signal::PadMode::Even;
signal::PadMode signal::PadMode::Constant;
signal::PadMode signal::PadMode::None;
```

## What It Does

The forward and backward passes start from rest, which creates a transient at each end. Extending the signal before filtering and cutting the extension afterwards moves the transient out of the kept samples. The pad mode chooses what the extension is.

## Values

| Value | Extension |
|---|---|
| `Odd` | Point reflection about the end sample: the signal continues its slope. |
| `Even` | Mirror reflection about the end sample. |
| `Constant` | The end sample repeated. |
| `None` | No extension; the transient stays in the output. |

## Returns

Field `pad` of `PhaseOpts`.

## Errors

`None` ignores the edge length; for the other modes an edge as long as the input is refused.

## Interpretation and Use

`Odd` continues a trend through the ends, so a ramp passes through a smoothing filter unchanged (the second example shows an error of zero), while `Even` and `Constant` bend toward the ends. Use `Odd` for data with a trend, `Even` for data that is flat at the ends and `None` only when the ends carry no information.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The first and last two samples of a ramp for each mode.

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
  const pluss::Series x = pluss::make_series({4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  for (const auto [label, pad] : {std::pair{"Odd     ", sig::PadMode::Odd}, {"Even    ", sig::PadMode::Even}, {"Constant", sig::PadMode::Constant}, {"None    ", sig::PadMode::None}}) {
    const auto y = sig::zero_phase(x, b, a, {.dim = 0, .edge = 3, .pad = pad}).items<double>();
    out.writeln(label, "first two =", y[0], y[1], " last two =", y[y.size() - 2], y.back());
  }
  out.writeln("a ramp from 4 to 13: Odd reproduces it, the others bend toward the ends");
}
```

Output

```text
Odd      first two = 4.0000 5.0000  last two = 12.0000 13.0000
Even     first two = 4.7500 5.1250  last two = 11.8750 12.2500
Constant first two = 4.3750 5.0625  last two = 11.9375 12.6250
None     first two = 4.3750 5.0625  last two = 11.7500 12.0000
a ramp from 4 to 13: Odd reproduces it, the others bend toward the ends
```
### Example 2

How far each mode moves a ramp from the input.

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
  const pluss::Series x = pluss::make_series({4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  for (const auto [label, pad] : {std::pair{"Odd     ", sig::PadMode::Odd}, {"Even    ", sig::PadMode::Even}, {"Constant", sig::PadMode::Constant}, {"None    ", sig::PadMode::None}}) {
    const auto y = sig::zero_phase(x, b, a, {.dim = 0, .edge = 3, .pad = pad}).items<double>();
    const auto v = x.items<double>();
    double worst = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) worst = std::max(worst, std::abs(y[i] - v[i]));
    out.writeln(label, "largest difference from the input ramp =", worst);
  }
}
```

Output

```text
Odd      largest difference from the input ramp = 0.0000
Even     largest difference from the input ramp = 0.7500
Constant largest difference from the input ramp = 0.3750
None     largest difference from the input ramp = 1.0000
```
### Example 3

The edge-length validation, and `None` ignoring it.

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
  refused(out, "Odd with an edge as long as the input", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0}), b, a, {.dim = 0, .edge = 4, .pad = sig::PadMode::Odd}); });
  refused(out, "None ignores the edge length", [&] { sig::zero_phase(pluss::make_series({1.0, 2.0, 3.0, 4.0}), b, a, {.dim = 0, .edge = 100, .pad = sig::PadMode::None}); });
}
```

Output

```text
Odd with an edge as long as the input : eng::signal::zero_phase: sample axis must be longer than the edge extension
None ignores the edge length : accepted
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConvMode.html">
    <span class="label">Previous</span>
    <span class="title">ConvMode</span>
  </a>
  <a class="next-link" href="FilterRun.html">
    <span class="label">Next</span>
    <span class="title">FilterRun</span>
  </a>
</div>
<!-- apiref:nav:end -->
