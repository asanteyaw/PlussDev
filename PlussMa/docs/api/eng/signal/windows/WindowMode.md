<!-- generated from WindowMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::WindowMode`

Chooses where a window places its endpoints.

## What It Does

A window of length `n` can be sampled over a span that ends on its last coefficient or one that stops just short of it. `Symmetric` samples `n` points over a full period so the window is its own mirror image: `w[i] = w[n-1-i]`. `Periodic` samples the first `n` points of a window one coefficient longer, so the next copy would start where this one ends and the window tiles.

## Values

| Value | Meaning |
|---|---|
| `Symmetric` | Mirror-symmetric, the right choice for filter design and for displaying a taper. |
| `Periodic` | The first `n` coefficients of the symmetric window of length `n + 1`, the right choice for spectral analysis with an `n`-point transform. |

## Errors

Every value is valid; the window functions validate their own size, dtype and platform.

## Interpretation and Use

The two modes differ by one coefficient of the underlying window, and the difference shrinks as `n` grows. For an `n`-point DFT, use `Periodic`: its spectrum matches the analytic window spectrum without the small asymmetry error that a symmetric window brings. For a filter designed by windowing an ideal impulse response, use `Symmetric` so that the filter keeps linear phase.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

For every window, the periodic form is the first n coefficients of the symmetric form of length n + 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  // A periodic window of length n is the first n coefficients of the symmetric window of length n + 1.
  const std::int64_t n = 16;
  using Maker = pluss::Series (*)(std::int64_t, const sig::WindowOptions &);
  const std::pair<const char *, Maker> makers[] = {{"boxcar", sig::boxcar}, {"bartlett", sig::bartlett}, {"blackman", sig::blackman}, {"hamming", sig::hamming}, {"hann", sig::hann}};
  for (const auto &[name, make] : makers) {
    const auto periodic = make(n, {.mode = sig::WindowMode::Periodic}).items<double>();
    const auto longer = make(n + 1, {}).items<double>();
    double worst = 0.0;
    for (std::int64_t i = 0; i < n; ++i) worst = std::max(worst, std::abs(periodic[i] - longer[i]));
    out.writeln(name, ": periodic(16) vs symmetric(17), largest difference =", worst);
  }
}
```

Output

```text
boxcar : periodic(16) vs symmetric(17), largest difference = 0.000000
bartlett : periodic(16) vs symmetric(17), largest difference = 0.000000
blackman : periodic(16) vs symmetric(17), largest difference = 0.000000
hamming : periodic(16) vs symmetric(17), largest difference = 0.000000
hann : periodic(16) vs symmetric(17), largest difference = 0.000000
```
### Example 2

The symmetric window is a mirror image and the periodic one is not; the periodic peak is exactly one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  // The symmetric window is its own mirror image; the periodic one is not, because its last coefficient is dropped.
  const std::int64_t n = 8;
  for (const auto mode : {sig::WindowMode::Symmetric, sig::WindowMode::Periodic}) {
    const auto w = sig::hann(n, {.mode = mode}).items<double>();
    double asymmetry = 0.0;
    for (std::int64_t i = 0; i < n; ++i) asymmetry = std::max(asymmetry, std::abs(w[i] - w[n - 1 - i]));
    out.writeln(mode == sig::WindowMode::Symmetric ? "symmetric" : "periodic ", ": largest |w[i] - w[n-1-i]| =", asymmetry, " peak =", *std::max_element(w.begin(), w.end()));
  }
}
```

Output

```text
symmetric : largest |w[i] - w[n-1-i]| = 0.000000  peak = 0.950484
periodic  : largest |w[i] - w[n-1-i]| = 0.353553  peak = 1.000000
```
### Example 3

The periodic Hann window overlap-adds to a constant; the symmetric one ripples.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));

  // A periodic window tiles: summed over shifts by n / 2 it is constant, which keeps the overlap-add of a half-overlapped analysis flat.
  const std::int64_t n = 16;
  for (const auto mode : {sig::WindowMode::Symmetric, sig::WindowMode::Periodic}) {
    const auto w = sig::hann(n, {.mode = mode}).items<double>();
    double lowest = 1e9, highest = -1e9;
    for (std::int64_t i = 0; i < n / 2; ++i) {
      const double total = w[i] + w[i + n / 2];
      lowest = std::min(lowest, total);
      highest = std::max(highest, total);
    }
    out.writeln(mode == sig::WindowMode::Symmetric ? "symmetric" : "periodic ", ": overlap-add of two half-shifted windows spans", lowest, "to", highest);
  }
}
```

Output

```text
symmetric : overlap-add of two half-shifted windows spans 0.897756 to 0.989074
periodic  : overlap-add of two half-shifted windows spans 1.000000 to 1.000000
```
## See Also

See the [Signal windows](../windows.md) family page for selection guidance and endpoint semantics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="WindowOptions.html">
    <span class="label">Next</span>
    <span class="title">WindowOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
