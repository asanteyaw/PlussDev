<!-- generated from to_sos.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::to_sos`

Realises a discrete pole-zero system as second-order sections.

## Signature

```cpp
pluss::Series lti::to_sos(const lti::PoleZero& system);
```

## What It Does

Pairs the poles and zeros into biquads and returns one row `b0 b1 b2 a0 a1 a2` per section, in the layout `signal::sos` consumes. A system of order `n` gives `ceil(n / 2)` sections, so an odd order uses one more section than half of it.

## Parameters

`system`
: A discrete `PoleZero`.

## Returns

A `Series` of shape `[sections, 6]`; `sections` is `ceil(order / 2)`.

## Errors

A continuous system and an improper system throw `pluss::Error`.

## Interpretation and Use

The cascade of the sections reproduces the response of the pole-zero model to rounding (the second example evaluates both at five frequencies). Pass the rows to `signal::sos` to filter.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The number of sections for orders 2 to 5.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const std::int64_t order : {2, 3, 4, 5}) {
    const lti::PoleZero design = sig::butter(order, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
    const pluss::Series sos = lti::to_sos(design);
    out.writeln("order", order, ": sections", sos.size(0), "by", sos.size(1));
  }
}
```

Output

```text
order 2 : sections 1 by 6
order 3 : sections 2 by 6
order 4 : sections 2 by 6
order 5 : sections 3 by 6
```
### Example 2

The cascade against the pole-zero response.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::PoleZero design = sig::butter(4, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const pluss::Series sections = lti::to_sos(design);
  const double pi = std::acos(-1.0);
  double worst = 0.0;
  for (const double hertz : {10.0, 50.0, 100.0, 200.0, 400.0}) {
    const double w = 2.0 * pi * hertz / 1000.0;
    std::complex<double> product = 1.0;
    for (std::int64_t s = 0; s < sections.size(0); ++s) {
      const auto row = sections.select(0, s).items<double>();
      product *= sig::freqz(pluss::make_series({row[0], row[1], row[2]}), pluss::make_series({row[3], row[4], row[5]}), pluss::make_series({w})).items<std::complex<double>>()[0];
    }
    const std::complex<double> direct = lti::freqresp(design, pluss::make_series({2.0 * pi * hertz})).items<std::complex<double>>()[0];
    worst = std::max(worst, std::abs(product - direct));
  }
  out.writeln("largest difference between the cascade and the pole-zero response =", worst);
}
```

Output

```text
largest difference between the cascade and the pole-zero response = 0.0000
```
### Example 3

The layout of one section.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::PoleZero design = sig::butter(2, 100.0, {.band = sig::Band::Low, .sample_rate = 1000.0});
  const pluss::Series sos = lti::to_sos(design);
  out.writeln("one section b0 b1 b2 a0 a1 a2 =", sos.flatten().items<double>());
  out.writeln("the denominator leader a0 is", sos.select(1, 3).items<double>()[0], "so the section can be run by signal::sos directly");
}
```

Output

```text
one section b0 b1 b2 a0 a1 a2 = [0.0675, 0.1349, 0.0675, 1.0000, -1.1430, 0.4128]
the denominator leader a0 is 1.0000 so the section can be run by signal::sos directly
```
### Example 4

Every refused system.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <optional>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
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

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  refused(out, "a continuous system", [] { lti::to_sos(lti::zpk(pluss::series::empty({0}, pluss::ScalarType::Float64), pluss::make_series({-1.0}), pluss::make_series(1.0))); });
  refused(out, "more zeros than poles", [] { lti::to_sos(lti::zpk(pluss::make_series({1.0, 2.0}), pluss::make_series({0.5}), pluss::make_series(1.0), 0.1)); });
}
```

Output

```text
a continuous system : eng::lti::to_sos: only discrete systems can be realized by the delay-form SOS filter
more zeros than poles : eng::lti::zpk: a proper system cannot have more zeros than poles
```
## See Also

See [Pole-zero and response](../response.md) for timebase, stability, and angular-frequency conventions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="zpk.html">
    <span class="label">Previous</span>
    <span class="title">zpk</span>
  </a>
  <a class="next-link" href="freqresp.html">
    <span class="label">Next</span>
    <span class="title">freqresp</span>
  </a>
</div>
<!-- apiref:nav:end -->
