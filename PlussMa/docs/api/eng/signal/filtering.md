<!-- generated from filtering.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Filtering and response

This family owns the sample-domain operations that combine signals with kernels, execute recursive filters, preserve continuation state, remove phase delay by forward-backward filtering, and evaluate coefficient frequency responses.

## Execution Model

Every operation works along a selected `Series` dimension. The default `dim = -1` means the final dimension, which matches ordinary one-dimensional signal vectors while allowing leading dimensions to represent independent batches. Floating and complex inputs are supported; integer samples are rejected rather than silently promoted under an unspecified policy.

`conv` and `corr` are alignment operations. `fir`, `filter`, and `sos` are causal filtering operations. `zero_phase` is deliberately non-causal because it runs in both directions. `freqz` does not filter samples at all; it evaluates the delay polynomial at requested angular frequencies.

## Streaming And State

`FilterRun` returns both values and terminal state. Feed that state into the next overload to process a long or arriving signal in blocks without restarting the recurrence. State belongs to the filter definition and batch shape, so changing coefficients, axis, or independent batch dimensions between blocks is invalid.

## Practical Examples

The examples establish convolution and response conventions, continue one recursive filter across two blocks, apply zero-phase filtering, and demonstrate dtype and denominator validation.

### Example 1

Compare full and same convolution, correlation, causal FIR output, and complex frequency response on one compact signal.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

namespace sig = pluss::subsp::eng::signal;
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

Carry recurrence state between blocks and contrast that causal workflow with a forward-backward zero-phase result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace sig = pluss::subsp::eng::signal;
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

Show the explicit failures for integer signal data and a direct-form denominator whose leading coefficient is zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <exception>
#include <string>

namespace sig = pluss::subsp::eng::signal;
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
## Related Pages

Use [Signal windows](windows.md) to construct common FIR tap shapes and [IIR design](design.md) to produce stable pole-zero systems before converting them to direct or second-order-section coefficients.

```{toctree}
:maxdepth: 1
:hidden:

filtering/ConvMode
filtering/PadMode
filtering/FilterRun
filtering/PhaseOpts
filtering/conv
filtering/corr
filtering/fir
filtering/filter
filtering/sos
filtering/zero_phase
filtering/freqz
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

filtering/ConvMode
filtering/PadMode
filtering/FilterRun
filtering/PhaseOpts
filtering/conv
filtering/corr
filtering/fir
filtering/filter
filtering/sos
filtering/zero_phase
filtering/freqz
```
<!-- apiref:toctree:end -->
