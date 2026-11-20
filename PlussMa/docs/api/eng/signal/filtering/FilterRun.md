<!-- generated from FilterRun.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::FilterRun`

The output of a filter and the state that continues it.

## Signature

```cpp
signal::FilterRun run = signal::filter(input, numerator, denominator);
```

## What It Does

Returned by `filter` and `sos`. `values` is the filtered signal; `state` is the filter's delay line after the last sample, with shape `[lanes..., order]` for `filter` (and `[sections, 2]` for `sos` on a single lane).

## Fields

| Field | Meaning |
|---|---|
| `values` | The output, with the shape of the input. |
| `state` | The final state, to pass to the stateful overload of the same filter. |

## Returns

Returned by `filter` and `sos`.

## Errors

A state is accepted only for compatible coefficients: a length different from the filter order is rejected, and the lane axes must broadcast.

## Interpretation and Use

Carry the state between blocks to filter an unbounded stream: the second example filters twelve samples as three blocks of four and reproduces the single pass to rounding. Reset the state to start a new signal.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The fields of a run, and the state length for a second-order filter.

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
  const pluss::Series b = pluss::make_series({0.5, 0.3, 0.2});
  const pluss::Series a = pluss::make_series({1.0, -0.4, 0.1});
  const sig::FilterRun run = sig::filter(x, b, a);
  out.writeln("values length", run.values.numel(), " state length", run.state.numel(), " for a filter of order 2");
  out.writeln("values =", run.values.items<double>());
  out.writeln("state  =", run.state.items<double>());
}
```

Output

```text
values length 4  state length 2  for a filter of order 2
values = [0.5000, 1.5000, 2.8500, 4.2900]
state  = [3.2310, 0.3710]
```
### Example 2

Three blocks against one pass.

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
  const pluss::Series b = pluss::make_series({0.5, 0.5});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  pluss::Series state;
  std::vector<double> all;
  for (int block = 0; block < 3; ++block) {
    const pluss::Series chunk = pluss::make_series({1.0 + block * 4, 2.0 + block * 4, 3.0 + block * 4, 4.0 + block * 4});
    const sig::FilterRun run = block == 0 ? sig::filter(chunk, b, a) : sig::filter(chunk, b, a, state);
    state = run.state;
    for (const double v : run.values.items<double>()) all.push_back(v);
  }
  const pluss::Series whole = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0});
  const auto once = sig::filter(whole, b, a).values.items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < once.size(); ++i) worst = std::max(worst, std::abs(once[i] - all[i]));
  out.writeln("three blocks of four samples against one pass of twelve, largest difference =", worst);
}
```

Output

```text
three blocks of four samples against one pass of twelve, largest difference = 0.0000
```
### Example 3

Which states are accepted and which are refused.

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
  const pluss::Series b = pluss::make_series({0.5, 0.5});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  const pluss::Series b3 = pluss::make_series({0.5, 0.3, 0.2});
  const sig::FilterRun first = sig::filter(pluss::make_series({1.0, 2.0, 3.0}), b, a);
  refused(out, "a state from a filter of a different order", [&] { sig::filter(pluss::make_series({1.0, 2.0, 3.0}), b3, a, first.state); });
  refused(out, "a state of the right order", [&] { sig::filter(pluss::make_series({4.0, 5.0, 6.0}), b, a, first.state); });
  // One state is shared by every lane: it broadcasts over a batch of rows.
  const sig::FilterRun rows = sig::filter(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3}), b, a, first.state, 1);
  out.writeln("one state shared by two rows: state shape", rows.state.size(0), "x", rows.state.size(1));
}
```

Output

```text
a state from a filter of a different order : eng::signal::filter: state length 1 does not match the filter order 2
a state of the right order : accepted
one state shared by two rows: state shape 2 x 1
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PadMode.html">
    <span class="label">Previous</span>
    <span class="title">PadMode</span>
  </a>
  <a class="next-link" href="PhaseOpts.html">
    <span class="label">Next</span>
    <span class="title">PhaseOpts</span>
  </a>
</div>
<!-- apiref:nav:end -->
