<!-- generated from WindowOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::WindowOptions`

The endpoint convention, dtype and platform of a window.

## What It Does

Passed as the second argument of `boxcar`, `bartlett`, `blackman`, `hamming` and `hann`. It does not change the shape of the window; it chooses which of the equivalent samplings is returned and where and in which precision the coefficients live.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `mode` | `signal::WindowMode` | `Symmetric` | `Symmetric` or `Periodic`; see `WindowMode`. |
| `dtype` | `pluss::ScalarType` | `Float64` | `Float32` or `Float64`. |
| `platform` | `pluss::Platform` | `CPU` | Where the result is allocated. |

## Errors

A dtype other than `Float32` or `Float64`, and a platform that cannot hold the dtype, are rejected by the window function that receives the options.

## Interpretation and Use

Construct options with designated initializers, setting only what differs from the default: `sig::hann(256, {.mode = sig::WindowMode::Periodic})`. Use `Float32` when the window multiplies float32 data so that no promotion to float64 happens, and note that the MPS holds only 32-bit and narrower floats.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The default options, and what a window made without any is.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  const sig::WindowOptions defaults;
  out.writeln("default mode is symmetric:", defaults.mode == sig::WindowMode::Symmetric);
  out.writeln("default dtype =", defaults.dtype);
  out.writeln("default platform is the CPU:", defaults.platform == pluss::Platform::CPU);
  const pluss::Series w = sig::blackman(5);
  out.writeln("a window made with no options has dtype", w.scalar_type(), "and", w.numel(), "coefficients");
}
```

Output

```text
default mode is symmetric: true
default dtype = float64
default platform is the CPU: true
a window made with no options has dtype float64 and 5 coefficients
```
### Example 2

Float32 against Float64: the dtype changes and the values agree to rounding.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace sig = pluss::subsp::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));

  const pluss::Series wide = sig::hamming(8);
  const pluss::Series narrow = sig::hamming(8, {.dtype = pluss::ScalarType::Float32});
  out.writeln("float64 dtype =", wide.scalar_type(), " float32 dtype =", narrow.scalar_type());
  const auto a = wide.items<double>();
  const auto b = narrow.items<float>();
  double worst = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) worst = std::max(worst, std::abs(a[i] - static_cast<double>(b[i])));
  out.writeln("largest float32 rounding difference =", worst);
}
```

Output

```text
float64 dtype = float64  float32 dtype = float32
largest float32 rounding difference = 0.00000009
```
### Example 3

Which dtype and platform combinations are refused.

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
  refused(out, "dtype Int64", [] { sig::hann(4, {.dtype = pluss::ScalarType::Int64}); });
  refused(out, "dtype Bool", [] { sig::hann(4, {.dtype = pluss::ScalarType::Bool}); });
  refused(out, "dtype Float32", [] { sig::hann(4, {.dtype = pluss::ScalarType::Float32}); });
  refused(out, "float64 on the MPS", [] { sig::hann(4, {.platform = pluss::Platform::MPS}); });
  refused(out, "float32 on the MPS", [] { sig::hann(4, {.dtype = pluss::ScalarType::Float32, .platform = pluss::Platform::MPS}); });
}
```

Output

```text
dtype Int64 : eng::signal::hann: dtype must be Float32 or Float64
dtype Bool : eng::signal::hann: dtype must be Float32 or Float64
dtype Float32 : accepted
float64 on the MPS : Series::to(MPS): float64 is unsupported on MPS; cast to float32/float16/bfloat16 first
float32 on the MPS : accepted
```
## See Also

See the [Signal windows](../windows.md) family page for selection guidance and endpoint semantics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="WindowMode.html">
    <span class="label">Previous</span>
    <span class="title">WindowMode</span>
  </a>
  <a class="next-link" href="boxcar.html">
    <span class="label">Next</span>
    <span class="title">boxcar</span>
  </a>
</div>
<!-- apiref:nav:end -->
