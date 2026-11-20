<!-- generated from conv.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::conv`

Convolves a signal with a kernel along one axis.

## Signature

```cpp
pluss::Series signal::conv(
    const pluss::Series& input,
    const pluss::Series& kernel,
    std::int64_t dim = -1,
    signal::ConvMode mode = signal::ConvMode::Full);
```

## What It Does

Computes `y[n] = sum_m k[m] x[n - m]` along `dim` for every lane of the other axes. The mode chooses how much of the result is kept: `Full` keeps every overlap, `Same` keeps the part aligned with the input and `Valid` keeps only the positions where the kernel lies entirely inside the signal. Convolution is commutative, so exchanging the input and the kernel gives the same sequence.

## Parameters

`input`
: A floating or complex `Series`. Integer and Bool inputs are rejected.

`kernel`
: A rank-one floating or complex `Series`, the impulse response or weights.

`dim`
: The axis to convolve along. Negative values count from the end; the default `-1` is the last axis.

`mode`
: A `ConvMode`: `Full` (default), `Same` or `Valid`.

## Returns

A `Series` of the promoted dtype. Along `dim` its length is `N + K - 1` for `Full`, `N` for `Same` and `N - K + 1` for `Valid` (for input length `N` and kernel length `K`); other axes keep their shape.

## Errors

An integer input, an empty input or kernel, and an axis out of range throw `pluss::Error`.

## Interpretation and Use

Use `conv` for FIR smoothing and differencing. The first and last `K - 1` samples of a `Full` result are start-up and shut-down transients where the kernel only partly overlaps the signal.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The three modes on a short signal, with the length each one returns.

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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  const pluss::Series k = pluss::make_series({1.0, 0.0, -1.0});
  for (const auto [label, mode] : {std::pair{"full ", sig::ConvMode::Full}, {"same ", sig::ConvMode::Same}, {"valid", sig::ConvMode::Valid}}) {
    const pluss::Series y = sig::conv(x, k, -1, mode);
    out.writeln(label, "length", y.numel(), "values", y.items<double>());
  }
}
```

Output

```text
full  length 7 values [1.0000, 2.0000, 2.0000, 2.0000, 2.0000, -4.0000, -5.0000]
same  length 5 values [2.0000, 2.0000, 2.0000, 2.0000, -4.0000]
valid length 3 values [2.0000, 2.0000, 2.0000]
```
### Example 2

The result against the direct sum, the commutative property, and a batch of rows convolved along an axis.

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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  const pluss::Series k = pluss::make_series({0.5, 1.0, 0.25});
  const auto y = sig::conv(x, k).items<double>();
  const auto xs = x.items<double>();
  const auto ks = k.items<double>();
  double worst = 0.0;
  for (std::size_t n = 0; n < y.size(); ++n) {
    double sum = 0.0;
    for (std::size_t m = 0; m < ks.size(); ++m)
      if (n >= m && n - m < xs.size()) sum += ks[m] * xs[n - m];
    worst = std::max(worst, std::abs(sum - y[n]));
  }
  out.writeln("largest difference from the direct sum =", worst);
  const auto swapped = sig::conv(k, x).items<double>();
  double commute = 0.0;
  for (std::size_t n = 0; n < y.size(); ++n) commute = std::max(commute, std::abs(swapped[n] - y[n]));
  out.writeln("conv(x, k) against conv(k, x), largest difference =", commute);
  const pluss::Series rows = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3});
  const pluss::Series two = sig::conv(rows, pluss::make_series({1.0, 1.0}), 1);
  out.writeln("two rows convolved along the last axis, shape", two.size(0), "x", two.size(1), "first row", two.select(0, 0).items<double>());
}
```

Output

```text
largest difference from the direct sum = 0.0000
conv(x, k) against conv(k, x), largest difference = 0.0000
two rows convolved along the last axis, shape 2 x 4 first row [1.0000, 3.0000, 5.0000, 3.0000]
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
  refused(out, "an integer input", [] { sig::conv(pluss::make_series({1, 2, 3}), pluss::make_series({1.0, 1.0})); });
  refused(out, "an empty kernel", [] { sig::conv(pluss::make_series({1.0, 2.0}), pluss::series::zeros({0}, pluss::ScalarType::Float64)); });
  refused(out, "an axis out of range", [] { sig::conv(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}), 3); });
}
```

Output

```text
an integer input : eng::signal::conv: input must have a floating or complex dtype
an empty kernel : eng::signal::conv: sample axes must be non-empty
an axis out of range : eng::signal::conv: dim out of range
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PhaseOpts.html">
    <span class="label">Previous</span>
    <span class="title">PhaseOpts</span>
  </a>
  <a class="next-link" href="corr.html">
    <span class="label">Next</span>
    <span class="title">corr</span>
  </a>
</div>
<!-- apiref:nav:end -->
