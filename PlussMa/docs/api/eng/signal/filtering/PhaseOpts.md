<!-- generated from PhaseOpts.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::PhaseOpts`

The axis, extension length and pad mode of `zero_phase`.

## Signature

```cpp
signal::PhaseOpts options{};
```

## What It Does

Groups the three settings of a zero-phase pass: which axis to filter along, how many samples to extend the signal by at each end, and how to extend it.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `dim` | The axis to filter along. | `-1` |
| `edge` | Extension length in samples; `-1` chooses three times the filter order. | `-1` |
| `pad` | A `PadMode`. | `Odd` |

## Returns

Argument of `zero_phase`.

## Errors

An invalid axis, an `edge` below `-1` and an extension at least as long as the axis throw `pluss::Error`.

## Interpretation and Use

The automatic edge is `3 *` the filter order; the first example checks that it equals an explicit edge of 6 for a second-order filter. Build the options with designated initializers: `{.dim = 0, .edge = 3}`.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The defaults, and the automatic edge against an explicit one.

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
  const sig::PhaseOpts defaults;
  out.writeln("default axis =", defaults.dim, " default edge =", defaults.edge, " default pad is Odd:", defaults.pad == sig::PadMode::Odd);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 4.0, 3.0, 5.0, 4.0, 6.0, 5.0, 7.0, 6.0, 8.0, 7.0, 9.0, 8.0, 10.0, 9.0});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const auto automatic = sig::zero_phase(x, b, a).items<double>();
  const auto explicit_edge = sig::zero_phase(x, b, a, {.dim = 0, .edge = 6}).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < automatic.size(); ++i) worst = std::max(worst, std::abs(automatic[i] - explicit_edge[i]));
  out.writeln("default options against edge 6 (three times the filter order of 2), largest difference =", worst);
}
```

Output

```text
default axis = -1  default edge = -1  default pad is Odd: true
default options against edge 6 (three times the filter order of 2), largest difference = 0.0000
```
### Example 2

Filtering along the second axis of a batch.

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
  const pluss::Series rows = pluss::make_series({1.0, 2.0, 4.0, 3.0, 5.0, 4.0, 6.0, 5.0, 10.0, 20.0, 40.0, 30.0, 50.0, 40.0, 60.0, 50.0}).reshape({2, 8});
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0});
  const pluss::Series y = sig::zero_phase(rows, b, a, {.dim = 1, .edge = 3});
  out.writeln("output shape", y.size(0), "x", y.size(1));
  out.writeln("row 0 =", y.select(0, 0).items<double>());
  out.writeln("row 1 =", y.select(0, 1).items<double>(), " (ten times row 0)");
}
```

Output

```text
output shape 2 x 8
row 0 = [1.0000, 2.1875, 3.1250, 3.7500, 4.2500, 4.7500, 5.0625, 5.0000]
row 1 = [10.0000, 21.8750, 31.2500, 37.5000, 42.5000, 47.5000, 50.6250, 50.0000]  (ten times row 0)
```
### Example 3

Every refused value.

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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  refused(out, "an axis out of range", [&] { sig::zero_phase(x, b, a, {.dim = 3}); });
  refused(out, "an edge below -1", [&] { sig::zero_phase(x, b, a, {.dim = 0, .edge = -5}); });
  refused(out, "an edge longer than the axis", [&] { sig::zero_phase(x, b, a, {.dim = 0, .edge = 9}); });
}
```

Output

```text
an axis out of range : eng::signal::zero_phase: dim out of range
an edge below -1 : eng::signal::zero_phase: edge must be -1 or nonnegative
an edge longer than the axis : eng::signal::zero_phase: sample axis must be longer than the edge extension
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="FilterRun.html">
    <span class="label">Previous</span>
    <span class="title">FilterRun</span>
  </a>
  <a class="next-link" href="conv.html">
    <span class="label">Next</span>
    <span class="title">conv</span>
  </a>
</div>
<!-- apiref:nav:end -->
