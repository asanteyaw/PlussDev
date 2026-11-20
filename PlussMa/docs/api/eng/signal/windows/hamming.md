<!-- generated from hamming.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::hamming`

Creates the Hamming window, a raised cosine that does not reach zero at the ends.

## Signature

```cpp
pluss::Series signal::hamming(
    std::int64_t size,
    const signal::WindowOptions& options = {});
```

## What It Does

The symmetric Hamming window is `w[i] = 0.54 - 0.46 cos(2 pi i / (n - 1))`. The coefficient 0.54 instead of 0.5 leaves a pedestal of 0.08 at both ends, visible as the first and last coefficient in the examples.

## Parameters

`size`
: Number of coefficients. Zero returns an empty rank-one `Series`; one returns the single coefficient `1`.

`options`
: A `WindowOptions`: the endpoint convention (`mode`), the floating dtype and the platform. The default is a symmetric float64 window on the CPU.

## Returns

A rank-one floating `Series` of length `size`, allocated with the requested dtype and platform.

## Errors

A negative `size`, a dtype other than `Float32` or `Float64`, and a platform that cannot hold the dtype (float64 on the MPS) throw `pluss::Error`.

## Interpretation and Use

Because the ends do not vanish, a Hamming-windowed frame still has a small step at its edges. The coherent gain tends to 0.54. The fourth example measures a main lobe of 2.07 bins and a highest sidelobe 42.45 dB below it, about 11 dB better than Hann's 31.47 dB at nearly the same main-lobe width. Use it when the nearest sidelobe matters more than the far ones.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Eight coefficients in both modes, and the closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const std::int64_t n = 8;
  const pluss::Series symmetric = sig::hamming(n);
  const pluss::Series periodic = sig::hamming(n, {.mode = sig::WindowMode::Periodic});
  out.writeln("symmetric =", symmetric.items<double>());
  out.writeln("periodic  =", periodic.items<double>());

  // The symmetric window equals the closed form w[i] = f(i / (n - 1)).
  const auto values = symmetric.items<double>();
  double worst = 0.0;
  for (std::int64_t i = 0; i < n; ++i)
    worst = std::max(worst, std::abs(values[i] - (0.54 - 0.46 * std::cos(2.0 * M_PI * i / (n - 1)))));
  out.writeln("largest deviation from the closed form =", worst);
  out.writeln("first and last coefficient =", values.front(), values.back());
}
```

Output

```text
symmetric = [0.0800, 0.2532, 0.6424, 0.9544, 0.9544, 0.6424, 0.2532, 0.0800]
periodic  = [0.0800, 0.2147, 0.5400, 0.8653, 1.0000, 0.8653, 0.5400, 0.2147]
largest deviation from the closed form = 0.0000
first and last coefficient = 0.0800 0.0800
```
### Example 2

Coherent gain and mean square at three lengths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  // The mean is the coherent gain (the amplitude a window leaves a steady tone) and the mean square is the power it leaves in noise.
  for (const std::int64_t n : {8, 64, 512}) {
    const pluss::Series w = sig::hamming(n);
    out.writeln("n =", n, " coherent gain =", w.mean().item<double>(), " mean square =", (w * w).mean().item<double>(), " peak =", w.max().item<double>());
  }
}
```

Output

```text
n = 8  coherent gain = 0.4825  mean square = 0.3485  peak = 0.9544
n = 64  coherent gain = 0.5328  mean square = 0.3913  peak = 0.9994
n = 512  coherent gain = 0.5391  mean square = 0.3966  peak = 1.0000
```
### Example 3

Every refused input and the size-zero and size-one cases.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

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
  refused(out, "a negative size", [] { sig::hamming(-1); });
  refused(out, "an integer dtype", [] { sig::hamming(4, {.dtype = pluss::ScalarType::Int64}); });
  refused(out, "float64 on the MPS", [] { sig::hamming(4, {.platform = pluss::Platform::MPS}); });
  out.writeln("size zero is empty:", sig::hamming(0).numel() == 0);
  out.writeln("size one is a single coefficient:", sig::hamming(1).items<double>());
}
```

Output

```text
a negative size : eng::signal::hamming: size must be non-negative
an integer dtype : eng::signal::hamming: dtype must be Float32 or Float64
float64 on the MPS : Series::to(MPS): float64 is unsupported on MPS; cast to float32/float16/bfloat16 first
size zero is empty: true
size one is a single coefficient: [1]
```
### Example 4

The spectrum of a 64-point window: main-lobe width and highest sidelobe.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));

  // The spectrum of a 64-point window: the width of its main lobe and the level of its highest sidelobe, relative to the gain at zero frequency.
  const std::int64_t n = 64;
  const std::int64_t points = 8192;
  std::vector<double> grid(points);
  for (std::int64_t i = 0; i < points; ++i) grid[i] = std::acos(-1.0) * i / (points - 1);
  const pluss::Series w = sig::hamming(n);
  const auto h = sig::freqz(w, pluss::make_series(grid)).abs().items<double>();
  std::int64_t null = 1;
  while (null + 1 < points && h[null + 1] < h[null]) ++null;   // the first minimum ends the main lobe
  double sidelobe = 0.0;
  for (std::int64_t i = null; i < points; ++i) sidelobe = std::max(sidelobe, h[i]);
  out.writeln("main lobe ends at", grid[null] * n / (2.0 * std::acos(-1.0)), "bins from zero frequency; highest sidelobe", 20.0 * std::log10(sidelobe / h[0]), "dB below the main lobe");
}
```

Output

```text
main lobe ends at 2.07 bins from zero frequency; highest sidelobe -42.45 dB below the main lobe
```
## See Also

See the [Signal windows](../windows.md) family page for selection guidance and endpoint semantics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="blackman.html">
    <span class="label">Previous</span>
    <span class="title">blackman</span>
  </a>
  <a class="next-link" href="hann.html">
    <span class="label">Next</span>
    <span class="title">hann</span>
  </a>
</div>
<!-- apiref:nav:end -->
