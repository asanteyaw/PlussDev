<!-- generated from corr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::corr`

Correlates a signal with a template along one axis.

## Signature

```cpp
pluss::Series signal::corr(
    const pluss::Series& input,
    const pluss::Series& kernel,
    std::int64_t dim = -1,
    signal::ConvMode mode = signal::ConvMode::Full);
```

## What It Does

Slides the kernel along the input without reversing it, so `corr(x, k)` equals `conv(x, flip(k))`. Where convolution measures how a system blends the past, correlation measures how closely a template matches the signal at each offset.

## Parameters

`input`
: A floating or complex `Series`.

`kernel`
: A rank-one floating or complex `Series`, the template.

`dim`
: The axis to correlate along; default `-1`.

`mode`
: A `ConvMode`: `Full` (default), `Same` or `Valid`.

## Returns

A `Series` with the lengths of `conv` for the same mode.

## Errors

An integer input, an empty input or kernel, and an axis out of range throw `pluss::Error`.

## Interpretation and Use

To locate a known pattern, correlate in `Valid` mode and take the position of the maximum: in `Valid` mode element `j` compares the template with the window starting at sample `j`, so the index of the peak is the start of the match. The peak value is the template energy when the match is exact.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Correlation against convolution with the flipped kernel.

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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  const pluss::Series k = pluss::make_series({1.0, 2.0, 4.0});
  out.writeln("corr(x, k)         =", sig::corr(x, k).items<double>());
  out.writeln("conv(x, flip(k))   =", sig::conv(x, k.flip(0)).items<double>());
  out.writeln("conv(x, k)         =", sig::conv(x, k).items<double>());
}
```

Output

```text
corr(x, k)         = [4.0000, 10.0000, 17.0000, 24.0000, 11.0000, 4.0000]
conv(x, flip(k))   = [4.0000, 10.0000, 17.0000, 24.0000, 11.0000, 4.0000]
conv(x, k)         = [1.0000, 4.0000, 11.0000, 18.0000, 20.0000, 16.0000]
```
### Example 2

Finding a pattern inside a longer signal from the peak of the correlation.

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
  const pluss::Series pattern = pluss::make_series({0.0, 1.0, 3.0, 1.0, 0.0});
  const pluss::Series signal = pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 3.0, 1.0, 0.0, 0.0, 0.0});
  const pluss::Series c = sig::corr(signal, pattern, -1, sig::ConvMode::Valid);
  const auto values = c.items<double>();
  const auto peak = std::max_element(values.begin(), values.end()) - values.begin();
  out.writeln("correlation values =", values);
  out.writeln("the pattern starts at sample", peak, "of the signal, where the correlation peaks at", values[peak]);
}
```

Output

```text
correlation values = [0.0000, 0.0000, 0.0000, 1.0000, 6.0000, 11.0000, 6.0000, 1.0000]
the pattern starts at sample 5 of the signal, where the correlation peaks at 11.0000
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
  refused(out, "an integer input", [] { sig::corr(pluss::make_series({1, 2, 3}), pluss::make_series({1.0, 1.0})); });
  refused(out, "an empty kernel", [] { sig::corr(pluss::make_series({1.0, 2.0}), pluss::series::zeros({0}, pluss::ScalarType::Float64)); });
  refused(out, "an axis out of range", [] { sig::corr(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}), 3); });
}
```

Output

```text
an integer input : eng::signal::corr: input must have a floating or complex dtype
an empty kernel : eng::signal::conv: sample axes must be non-empty
an axis out of range : eng::signal::corr: dim out of range
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conv.html">
    <span class="label">Previous</span>
    <span class="title">conv</span>
  </a>
  <a class="next-link" href="fir.html">
    <span class="label">Next</span>
    <span class="title">fir</span>
  </a>
</div>
<!-- apiref:nav:end -->
