<!-- generated from windows.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Signal windows

Window functions shape a finite observation before spectral estimation, FIR construction, or overlap processing. This family produces `Series` directly, so coefficients retain an explicit floating dtype and execution platform and can flow into the rest of Pluss without conversion.

## Choosing A Window

`boxcar` leaves every sample unchanged and therefore has the narrowest main lobe but the strongest spectral leakage. `bartlett` introduces a linear triangular taper. `hann` and `hamming` use raised cosines; Hann reaches zero at symmetric endpoints, while Hamming keeps small nonzero endpoints. `blackman` uses an additional cosine term for stronger sidelobe suppression at the cost of a wider main lobe.

The best window depends on the measurement goal. A stronger taper is not universally better: it trades frequency resolution for reduced leakage. Compare windows under the same length and endpoint convention rather than treating their coefficients as interchangeable decoration.

## Endpoint Convention

`WindowMode::Symmetric` creates a window symmetric over all returned coefficients and is the usual choice for finite impulse response design. `WindowMode::Periodic` creates the first `size` samples of a window whose next sample would close the period; it is usually the right choice before an `size`-point DFT.

## Practical Examples

The examples compare all supplied windows, demonstrate endpoint, dtype, empty, and singleton behavior, and show the validation failures that reject impossible requests.

### Example 1

Generate every built-in window at the same odd length so their taper shapes can be compared directly.

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

Contrast symmetric and periodic Hann coefficients while selecting a `Float32` output and inspecting edge sizes.

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

Exercise the two central construction guards: non-negative length and floating output dtype.

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
## Related Pages

Use [Filtering and response](filtering.md) to apply coefficients or inspect their frequency response, and [IIR design](design.md) when a recursive pole-zero design is required instead of an explicit windowed kernel.

```{toctree}
:maxdepth: 1
:hidden:

windows/WindowMode
windows/WindowOptions
windows/boxcar
windows/bartlett
windows/blackman
windows/hamming
windows/hann
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

windows/WindowMode
windows/WindowOptions
windows/boxcar
windows/bartlett
windows/blackman
windows/hamming
windows/hann
```
<!-- apiref:toctree:end -->
