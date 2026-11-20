<!-- generated from sos.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::sos`

Applies a cascade of second-order sections.

## Signature

```cpp
signal::FilterRun signal::sos(
    const pluss::Series& input,
    const pluss::Series& sections,
    std::int64_t dim = -1);

signal::FilterRun signal::sos(
    const pluss::Series& input,
    const pluss::Series& sections,
    const pluss::Series& state,
    std::int64_t dim = -1);
```

## What It Does

Filters through a cascade of biquads. `sections` has one row per section in the order `b0 b1 b2 a0 a1 a2`, and the output of each section feeds the next. The designs of `butter`, `cheby1` and `cheby2` convert to this layout through `lti::to_sos`.

## Parameters

`input`
: A floating or complex `Series`.

`sections`
: A `[section, 6]` `Series`; every `a0` (column 3) must be nonzero.

`state`
: Optional initial state, as returned in `FilterRun::state` by a previous run of the same sections.

`dim`
: The axis to filter along; default `-1`.

## Returns

A `FilterRun`: the cascade output and the state of every section.

## Errors

Sections that are not `[section, 6]`, a zero leading denominator coefficient in any section and an incompatible state throw `pluss::Error`.

## Interpretation and Use

The cascade of two sections gives the same samples as the single polynomial filter whose numerator and denominator are the products of the sections (the first example). Use the cascade for the designs from `signal::butter` and its relatives.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A two-section cascade against the single-polynomial filter.

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
  // Two sections b0 b1 b2 a0 a1 a2: a one-pole lowpass pair.
  const pluss::Series sections = pluss::make_series({0.5, 0.5, 0.0, 1.0, -0.2, 0.0, 0.5, 0.5, 0.0, 1.0, -0.4, 0.0}).reshape({2, 6});
  const pluss::Series x = pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const auto cascade = sig::sos(x, sections).values.items<double>();
  // The same filter as one polynomial: the products of the numerators and of the denominators.
  const pluss::Series b = pluss::make_series({0.25, 0.5, 0.25});
  const pluss::Series a = pluss::make_series({1.0, -0.6, 0.08});
  const auto direct = sig::filter(x, b, a).values.items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < cascade.size(); ++i) worst = std::max(worst, std::abs(cascade[i] - direct[i]));
  out.writeln("cascade impulse response =", cascade);
  out.writeln("single-polynomial response =", direct);
  out.writeln("largest difference =", worst);
}
```

Output

```text
cascade impulse response = [0.2500, 0.6500, 0.6200, 0.3200, 0.1424, 0.0598]
single-polynomial response = [0.2500, 0.6500, 0.6200, 0.3200, 0.1424, 0.0598]
largest difference = 0.0000
```
### Example 2

Block-wise filtering with the carried state.

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
  const pluss::Series sections = pluss::make_series({0.5, 0.5, 0.0, 1.0, -0.2, 0.0, 0.5, 0.5, 0.0, 1.0, -0.4, 0.0}).reshape({2, 6});
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const auto once = sig::sos(x, sections).values.items<double>();
  const sig::FilterRun first = sig::sos(x.slice(0, 0, 2), sections);
  const sig::FilterRun second = sig::sos(x.slice(0, 2, 6), sections, first.state);
  const auto joined = first.values.unite({second.values}, 0).items<double>();
  double worst = 0.0;
  for (std::size_t i = 0; i < once.size(); ++i) worst = std::max(worst, std::abs(once[i] - joined[i]));
  out.writeln("state shape: sections", first.state.size(0), "by", first.state.size(1));
  out.writeln("largest difference between one pass and two blocks =", worst);
}
```

Output

```text
state shape: sections 2 by 2
largest difference between one pass and two blocks = 0.0000
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
  refused(out, "sections of the wrong width", [] { sig::sos(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 0.0, 0.0, 1.0, 0.0}).reshape({1, 5})); });
  refused(out, "a zero leading denominator coefficient", [] { sig::sos(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 0.0, 0.0, 0.0, 0.0, 0.0}).reshape({1, 6})); });
  refused(out, "no sections", [] { sig::sos(pluss::make_series({1.0, 2.0}), pluss::series::zeros({0, 6}, pluss::ScalarType::Float64)); });
}
```

Output

```text
sections of the wrong width : eng::signal::sos: sections must have shape [section, 6]
a zero leading denominator coefficient : eng::signal::sos: every section denominator[0] must be nonzero
no sections : eng::signal::sos: sections must have shape [section, 6]
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="zero_phase.html">
    <span class="label">Next</span>
    <span class="title">zero_phase</span>
  </a>
</div>
<!-- apiref:nav:end -->
