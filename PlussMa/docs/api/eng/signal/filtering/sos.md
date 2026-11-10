<!-- generated from sos.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::sos`

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

Applies a cascade of biquadratic second-order sections. Cascades are generally more numerically robust than one high-order direct polynomial, especially for narrow or high-order IIR designs.

## Parameters

`input`
: Floating or complex samples.

`sections`
: Coefficient matrix with one six-coefficient row per section, conventionally `[b0,b1,b2,a0,a1,a2]`.

`state`
: Optional per-section continuation state from a compatible run.

`dim`
: Ordered sample axis.

## Returns

`FilterRun` containing the cascade output and terminal state for every section.

## Errors

Rejects malformed section shape, zero section denominator leaders, invalid axes, incompatible state, dtype, platform, or batch shape.

## Practical Examples

These verified programs show the normal contract, an important alternative or stateful workflow, and validation behavior.

### Example 1

Compare convolution, correlation, FIR filtering, and frequency response.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series input = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::Series kernel = pluss::make_series({1.0, 1.0});
  const pluss::Series omega = pluss::make_series({0.0, std::acos(-1.0)});

  out.writeln("full convolution =", sig::conv(input, kernel).items<double>());
  out.writeln("same convolution =",
              sig::conv(input, kernel, -1, sig::ConvMode::Same)
                  .items<double>());
  out.writeln("correlation =", sig::corr(input, kernel).items<double>());
  out.writeln("causal FIR =", sig::fir(input, kernel).items<double>());
  out.writeln("frequency response =", sig::freqz(kernel, omega).items<std::complex<double>>());
}
```

Output

```text
full convolution = [1.0000, 3.0000, 5.0000, 3.0000]
same convolution = [1.0000, 3.0000, 5.0000]
correlation = [1.0000, 3.0000, 5.0000, 3.0000]
causal FIR = [1.0000, 3.0000, 5.0000]
frequency response = [(2,0), (0,-1.22465e-16)]
```
### Example 2

Continue a recursive filter across blocks and apply forward-backward filtering.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::ma::eng::signal;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));

  const pluss::Series input =
      pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  const pluss::Series numerator = pluss::make_series({0.5, 0.5});
  const pluss::Series denominator = pluss::make_series({1.0, -0.5});
  const sig::FilterRun first =
      sig::filter(input.slice(0, 0, 4), numerator, denominator);
  const sig::FilterRun second = sig::filter(
      input.slice(0, 4, 8), numerator, denominator, first.state);
  const pluss::Series joined = first.values.unite({second.values}, 0);

  out.writeln("first block =", first.values.items<double>());
  out.writeln("carried state =", first.state.items<double>());
  out.writeln("second block =", second.values.items<double>());
  out.writeln("joined output =", joined.items<double>());

  const pluss::Series smooth = sig::zero_phase(
      input, numerator, denominator,
      {.dim = 0, .edge = 3, .pad = sig::PadMode::Odd});
  out.writeln("zero phase =", smooth.items<double>());
}
```

Output

```text
first block = [0.5000, 1.7500, 3.3750, 5.1875]
carried state = [4.5938]
second block = [7.0938, 9.0469, 11.0234, 13.0117]
joined output = [0.5000, 1.7500, 3.3750, 5.1875, 7.0938, 9.0469, 11.0234, 13.0117]
zero phase = [4.3691, 8.1758, 12.0703, 16.0000, 19.9297, 23.8242, 27.6309, 31.2530]
```
### Example 3

Observe validation of signal dtype and recurrence coefficients.

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
    static_cast<void>(sig::conv(pluss::make_series({1, 2, 3}),
                                   pluss::make_series({1.0, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("integer input rejected =",
                message.substr(0, message.find('\n')));
  }
  try {
    static_cast<void>(sig::filter(
        pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}),
        pluss::make_series({0.0, 1.0})));
  } catch (const std::exception &error) {
    const std::string message = error.what();
    out.writeln("zero denominator rejected =",
                message.substr(0, message.find('\n')));
  }
}
```

Output

```text
integer input rejected = eng::signal::conv: input must have a floating or complex dtype
zero denominator rejected = eng::signal::filter: denominator[0] must be nonzero
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
