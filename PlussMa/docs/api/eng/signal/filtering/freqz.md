<!-- generated from freqz.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::freqz`

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

Evaluates the complex delay-polynomial response at angular frequencies `omega`. The numerator-only overload is FIR; the second evaluates a rational IIR response.

## Parameters

`numerator`
: Feed-forward coefficients.

`denominator`
: Optional feedback coefficients with nonzero leading value.

`omega`
: Angular frequencies in radians per sample.

`dim`
: Coefficient axis when coefficients are batched.

## Returns

A complex `Series` whose frequency dimension follows `omega` and whose leading dimensions follow compatible coefficient batches.

## Errors

Rejects invalid coefficient axes, non-floating/non-complex inputs, zero leading denominator, and incompatible dtype, platform, or batch shape.

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
  <a class="prev-link" href="zero_phase.html">
    <span class="label">Previous</span>
    <span class="title">zero_phase</span>
  </a>
</div>
<!-- apiref:nav:end -->
