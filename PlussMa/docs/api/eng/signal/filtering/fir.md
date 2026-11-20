<!-- generated from fir.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::fir`

Applies a finite impulse response filter causally.

## Signature

```cpp
pluss::Series signal::fir(
    const pluss::Series& input,
    const pluss::Series& taps,
    std::int64_t dim = -1);
```

## What It Does

Convolves with the taps and keeps the first `N` samples, the causal part: `y[n] = sum_{m <= n} h[m] x[n - m]`, with the signal taken as zero before its first sample. The output has the same length as the input.

## Parameters

`input`
: A floating or complex `Series`.

`taps`
: A rank-one floating or complex `Series`, the impulse response.

`dim`
: The axis to filter along; default `-1`.

## Returns

A `Series` with the shape of `input`.

## Errors

An integer input, empty taps and an axis out of range throw `pluss::Error`.

## Interpretation and Use

The first `K - 1` outputs are the start-up transient of a `K`-tap filter. A causal filter delays features of the signal; use `zero_phase` when the delay is not acceptable.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A three-point moving average, with the start-up transient visible.

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
  const pluss::Series x = pluss::make_series({3.0, 6.0, 9.0, 12.0, 15.0});
  const pluss::Series taps = pluss::make_series({1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0});
  const pluss::Series y = sig::fir(x, taps);
  out.writeln("input length", x.numel(), " output length", y.numel());
  out.writeln("three-point moving average =", y.items<double>());
}
```

Output

```text
input length 5  output length 5
three-point moving average = [1.0000, 3.0000, 6.0000, 9.0000, 12.0000]
```
### Example 2

`fir` against the causal part of the full convolution.

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
  const pluss::Series x = pluss::make_series({1.0, -2.0, 3.0, -4.0, 5.0});
  const pluss::Series taps = pluss::make_series({0.5, 0.3, 0.2});
  const auto causal = sig::fir(x, taps).items<double>();
  const auto full = sig::conv(x, taps).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < causal.size(); ++i) worst = std::max(worst, std::abs(causal[i] - full[i]));
  out.writeln("fir against the first", causal.size(), "samples of the full convolution, largest difference =", worst);
  out.writeln("the full convolution has", full.size(), "samples; fir keeps the causal part");
}
```

Output

```text
fir against the first 5 samples of the full convolution, largest difference = 0.0000
the full convolution has 7 samples; fir keeps the causal part
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
  refused(out, "an integer input", [] { sig::fir(pluss::make_series({1, 2, 3}), pluss::make_series({1.0, 1.0})); });
  refused(out, "empty taps", [] { sig::fir(pluss::make_series({1.0, 2.0}), pluss::series::zeros({0}, pluss::ScalarType::Float64)); });
  refused(out, "an axis out of range", [] { sig::fir(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}), 4); });
}
```

Output

```text
an integer input : eng::signal::fir: input must have a floating or complex dtype
empty taps : eng::signal::conv: sample axes must be non-empty
an axis out of range : eng::signal::fir: dim out of range
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr.html">
    <span class="label">Previous</span>
    <span class="title">corr</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
<!-- apiref:nav:end -->
