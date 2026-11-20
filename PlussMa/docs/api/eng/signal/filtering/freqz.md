<!-- generated from freqz.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::freqz`

Evaluates a filter's frequency response on a grid of angular frequencies.

## Signature

```cpp
pluss::Series signal::freqz(
    const pluss::Series& numerator,
    const pluss::Series& omega,
    std::int64_t dim = -1);

pluss::Series signal::freqz(
    const pluss::Series& numerator,
    const pluss::Series& denominator,
    const pluss::Series& omega,
    std::int64_t dim = -1);
```

## What It Does

Returns `H(e^{j w})` for each `w` in `omega`: the numerator polynomial in `e^{-j w}` divided by the denominator polynomial when one is given. The result is complex with the grid's shape. `omega` is in radians per sample from 0 to `pi`.

## Parameters

`numerator`
: Rank-one coefficients `b` (floating or complex).

`denominator`
: Optional rank-one coefficients `a`; omitted means 1.

`omega`
: A `Series` with at least one dimension of angular frequencies, rad/sample.

`dim`
: The coefficient axis; default `-1`.

## Returns

A complex `Series` of the shape of `omega`.

## Errors

An integer coefficient array, an empty numerator and a scalar frequency grid throw `pluss::Error`.

## Interpretation and Use

The magnitude at `w = 0` is the DC gain and at `w = pi` the gain at the Nyquist rate. The first example checks the two-tap average against `|cos(w / 2)|` and the second a one-pole filter against its closed form `1 / (1 - 0.5 e^{-j w})`.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A two-tap average against `|cos(w / 2)|`.

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
  const pluss::Series taps = pluss::make_series({0.5, 0.5});
  const double pi = std::acos(-1.0);
  const pluss::Series omega = pluss::make_series({0.0, pi / 2.0, pi});
  const auto h = sig::freqz(taps, omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < h.size(); ++i) {
    const double omega_i = i == 0 ? 0.0 : (i == 1 ? pi / 2.0 : pi);
    out.writeln("omega", omega_i, ": |H| =", std::abs(h[i]), " expected |cos(omega / 2)| =", std::abs(std::cos(omega_i / 2.0)));
  }
}
```

Output

```text
omega 0.0000 : |H| = 1.0000  expected |cos(omega / 2)| = 1.0000
omega 1.5708 : |H| = 0.7071  expected |cos(omega / 2)| = 0.7071
omega 3.1416 : |H| = 0.0000  expected |cos(omega / 2)| = 0.0000
```
### Example 2

A one-pole filter against its closed form.

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
  const pluss::Series b = pluss::make_series({1.0});
  const pluss::Series a = pluss::make_series({1.0, -0.5});
  const double pi = std::acos(-1.0);
  const pluss::Series omega = pluss::make_series({0.0, pi / 2.0, pi});
  const auto h = sig::freqz(b, a, omega).items<std::complex<double>>();
  for (std::size_t i = 0; i < h.size(); ++i) {
    const double omega_i = i == 0 ? 0.0 : (i == 1 ? pi / 2.0 : pi);
    const std::complex<double> expected = 1.0 / (1.0 - 0.5 * std::exp(std::complex<double>(0.0, -omega_i)));
    out.writeln("omega", omega_i, ": |H| =", std::abs(h[i]), " closed form =", std::abs(expected));
  }
}
```

Output

```text
omega 0.0000 : |H| = 2.0000  closed form = 2.0000
omega 1.5708 : |H| = 0.8944  closed form = 0.8944
omega 3.1416 : |H| = 0.6667  closed form = 0.6667
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
  const pluss::Series omega = pluss::make_series({0.0, 1.0});
  refused(out, "an integer numerator", [&] { sig::freqz(pluss::make_series({1, 2}), omega); });
  refused(out, "an empty numerator", [&] { sig::freqz(pluss::series::zeros({0}, pluss::ScalarType::Float64), omega); });
  refused(out, "a scalar frequency grid", [&] { sig::freqz(pluss::make_series({1.0, 1.0}), pluss::make_series(0.5)); });
}
```

Output

```text
an integer numerator : eng::signal::freqz: numerator must have a floating or complex dtype
an empty numerator : eng::signal::freqz: coefficient axis must be non-empty
a scalar frequency grid : eng::signal::freqz: omega must have at least one dimension
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="zero_phase.html">
    <span class="label">Previous</span>
    <span class="title">zero_phase</span>
  </a>
</div>
<!-- apiref:nav:end -->
