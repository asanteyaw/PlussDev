<!-- generated from filter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::filter`

Applies a rational filter by recursion and returns the state for continuation.

## Signature

```cpp
signal::FilterRun signal::filter(
    const pluss::Series& input,
    const pluss::Series& numerator,
    const pluss::Series& denominator,
    std::int64_t dim = -1);

signal::FilterRun signal::filter(
    const pluss::Series& input,
    const pluss::Series& numerator,
    const pluss::Series& denominator,
    const pluss::Series& state,
    std::int64_t dim = -1);
```

## What It Does

Runs the difference equation `a[0] y[n] = sum b[k] x[n-k] - sum_{k>=1} a[k] y[n-k]` in transposed direct form II. The result is a `FilterRun`: the output and the final delay-line state, which the stateful overload accepts to continue the same filter on the next block of samples with no seam.

## Parameters

`input`
: A floating or complex `Series`.

`numerator`, `denominator`
: Rank-one coefficient `Series` `b` and `a`. `a[0]` must be nonzero. The filter order is `max(len(b), len(a)) - 1`.

`state`
: Optional initial state of shape `[lanes..., order]`; omitted means rest. Lane axes may broadcast; the state axis must equal the order.

`dim`
: The axis to filter along; default `-1`.

## Returns

A `FilterRun` with `values` (the input shape) and `state` (`[lanes..., order]`).

## Errors

An integer input, a zero `denominator[0]`, a state whose length is not the filter order, a state that does not broadcast to the lanes and an axis out of range throw `pluss::Error`.

## Interpretation and Use

The impulse response of `y[n] = x[n] + 0.5 y[n-1]` is `0.5^n`, which the first example checks. Processing a long recording in blocks with the carried state gives the same samples as one pass, to rounding.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The impulse response of a one-pole filter against its closed form.

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
  const pluss::Series x = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const pluss::Series b = pluss::make_series({1.0});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  const auto y = sig::filter(x, b, a).values.items<double>();
  out.writeln("impulse response of y[n] = x[n] + 0.5 y[n-1] =", y);
  double worst = 0.0, expected = 1.0;
  for (std::size_t n = 0; n < y.size(); ++n, expected *= 0.5) worst = std::max(worst, std::abs(y[n] - expected));
  out.writeln("largest difference from 0.5^n =", worst);
}
```

Output

```text
impulse response of y[n] = x[n] + 0.5 y[n-1] = [1.0000, 0.5000, 0.2500, 0.1250, 0.0625, 0.0312]
largest difference from 0.5^n = 0.0000
```
### Example 2

Two blocks with a carried state against one pass.

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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  const pluss::Series b = pluss::make_series({0.5, 0.5});
  const pluss::Series a = pluss::make_series({1.0, -0.3});
  const auto once = sig::filter(x, b, a).values.items<double>();
  const sig::FilterRun first = sig::filter(x.slice(0, 0, 3), b, a);
  const sig::FilterRun second = sig::filter(x.slice(0, 3, 8), b, a, first.state);
  const auto joined = first.values.unite({second.values}, 0).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < once.size(); ++i) worst = std::max(worst, std::abs(once[i] - joined[i]));
  out.writeln("one pass       =", once);
  out.writeln("two blocks     =", joined);
  out.writeln("largest difference between the two =", worst);
}
```

Output

```text
one pass       = [0.5000, 1.6500, 2.9950, 4.3985, 5.8195, 7.2459, 8.6738, 10.1021]
two blocks     = [0.5000, 1.6500, 2.9950, 4.3985, 5.8195, 7.2459, 8.6738, 10.1021]
largest difference between the two = 0.0000
```
### Example 3

A batch of rows filtered along an axis, with the per-row state.

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
  const pluss::Series rows = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}).reshape({2, 4});
  const sig::FilterRun run = sig::filter(rows, pluss::make_series({1.0}), pluss::make_series({1.0, -0.5}), 1);
  out.writeln("values shape", run.values.size(0), "x", run.values.size(1), " state shape", run.state.size(0), "x", run.state.size(1));
  out.writeln("row 0 =", run.values.select(0, 0).items<double>());
  out.writeln("row 1 =", run.values.select(0, 1).items<double>(), " (ten times row 0)");
}
```

Output

```text
values shape 2 x 4  state shape 2 x 1
row 0 = [1.0000, 2.5000, 4.2500, 6.1250]
row 1 = [10.0000, 25.0000, 42.5000, 61.2500]  (ten times row 0)
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fir.html">
    <span class="label">Previous</span>
    <span class="title">fir</span>
  </a>
  <a class="next-link" href="sos.html">
    <span class="label">Next</span>
    <span class="title">sos</span>
  </a>
</div>
<!-- apiref:nav:end -->
