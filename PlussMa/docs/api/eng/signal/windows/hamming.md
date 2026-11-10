<!-- generated from hamming.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::hamming`

## Signature

```cpp
pluss::Series signal::hamming(
    std::int64_t size,
    const signal::WindowOptions& options = {});
```

## What It Does

Creates a raised-cosine Hamming window. Unlike Hann, symmetric Hamming endpoints remain nonzero, reducing the discontinuity without forcing samples completely to zero.

## Parameters

`size`
: Number of coefficients to return. Zero returns an empty rank-one `Series`; one returns the mathematically useful singleton coefficient.

`options`
: Endpoint, dtype, and platform policy. Hamming is often selected where sidelobe suppression matters but discarding endpoint contribution is undesirable.

## Returns

A rank-one floating `Series` of length `size`, allocated with the requested dtype and platform.

## Errors

Rejects a negative `size`, a non-floating dtype, or an unsupported platform request.

## Practical Examples

These verified programs show the normal contract, an important alternative or stateful workflow, and validation behavior.

### Example 1

Generate and compare every built-in window at a common length.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  out.writeln("boxcar =", sig::boxcar(5).items<double>());
  out.writeln("bartlett =", sig::bartlett(5).items<double>());
  out.writeln("blackman =", sig::blackman(5).items<double>());
  out.writeln("hamming =", sig::hamming(5).items<double>());
  out.writeln("hann =", sig::hann(5).items<double>());
}
```

Output

```text
boxcar = [1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
bartlett = [0.0000, 0.5000, 1.0000, 0.5000, 0.0000]
blackman = [0.0000, 0.3400, 1.0000, 0.3400, 0.0000]
hamming = [0.0800, 0.5400, 1.0000, 0.5400, 0.0800]
hann = [0.0000, 0.5000, 1.0000, 0.5000, 0.0000]
```
### Example 2

Switch endpoint convention and dtype, then inspect zero-length and singleton behavior.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series symmetric = sig::hann(6);
  const pluss::Series periodic = sig::hann(
      6, {.mode = sig::WindowMode::Periodic,
          .dtype = pluss::ScalarType::Float32});

  out.writeln("symmetric =", symmetric.items<double>());
  out.writeln("periodic =", periodic.items<float>());
  out.writeln("periodic dtype =", periodic.scalar_type());
  out.writeln("empty size =", sig::hann(0).numel());
  out.writeln("singleton =", sig::hann(1).items<double>());
}
```

Output

```text
symmetric = [0.0000, 0.3455, 0.9045, 0.9045, 0.3455, 0.0000]
periodic = [0.0000, 0.2500, 0.7500, 1.0000, 0.7500, 0.2500]
periodic dtype = float32
empty size = 0
singleton = [1.0000]
```
### Example 3

Observe the validation errors for invalid size and dtype requests.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  try {
    static_cast<void>(sig::hann(-1));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("negative size rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::hamming(
        4, {.dtype = pluss::ScalarType::Int64}));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("integer dtype rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
negative size rejected = eng::signal::hann: size must be non-negative
integer dtype rejected = eng::signal::hamming: dtype must be Float32 or Float64
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
