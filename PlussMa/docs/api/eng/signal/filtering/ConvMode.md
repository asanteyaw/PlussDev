<!-- generated from ConvMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `signal::ConvMode`

How much of a convolution or correlation is kept.

## Signature

```cpp
signal::ConvMode signal::ConvMode::Full;
signal::ConvMode signal::ConvMode::Same;
signal::ConvMode signal::ConvMode::Valid;
```

## What It Does

A convolution of a length-`N` signal with a length-`K` kernel has `N + K - 1` overlaps. `Full` keeps all of them, `Same` keeps the `N` centred on the input, and `Valid` keeps the `N - K + 1` where the kernel lies entirely inside the signal.

## Values

| Value | Length | Meaning |
|---|---|---|
| `Full` | `N + K - 1` | Every overlap, including the start-up and shut-down transients. |
| `Same` | `N` | The centre of the full result, aligned with the input. |
| `Valid` | `N - K + 1` | Only the positions with complete overlap. |

## Returns

Argument of `conv` and `corr`.

## Errors

Every value is valid; the functions validate their own arguments. When the kernel is longer than the input, the two sequences trade roles and the lengths follow (the third example).

## Interpretation and Use

`Same` is the choice when the output is compared sample for sample with the input, `Valid` when edge effects must not enter the result, `Full` when the complete convolution is wanted, for example to build a cascade kernel.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The length of each mode for two input lengths.

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
  for (const std::int64_t n : {5, 8}) {
    for (const std::int64_t m : {3}) {
      const pluss::Series x = pluss::series::ones({n}, pluss::ScalarType::Float64);
      const pluss::Series k = pluss::series::ones({m}, pluss::ScalarType::Float64);
      out.writeln("input", n, "kernel", m, ": Full", sig::conv(x, k, -1, sig::ConvMode::Full).numel(), " Same", sig::conv(x, k, -1, sig::ConvMode::Same).numel(), " Valid", sig::conv(x, k, -1, sig::ConvMode::Valid).numel());
    }
  }
}
```

Output

```text
input 5 kernel 3 : Full 7  Same 5  Valid 3
input 8 kernel 3 : Full 10  Same 8  Valid 6
```
### Example 2

The three modes on one signal, showing that `Same` is the centre of `Full`.

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
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0});
  const pluss::Series k = pluss::make_series({1.0, 1.0, 1.0});
  const auto full = sig::conv(x, k, -1, sig::ConvMode::Full).items<double>();
  const auto same = sig::conv(x, k, -1, sig::ConvMode::Same).items<double>();
  const auto valid = sig::conv(x, k, -1, sig::ConvMode::Valid).items<double>();
  out.writeln("full  =", full);
  out.writeln("same  =", same, " (the centre of full)");
  out.writeln("valid =", valid, " (only where the kernel fits entirely)");
}
```

Output

```text
full  = [1.0000, 3.0000, 6.0000, 9.0000, 12.0000, 9.0000, 5.0000]
same  = [3.0000, 6.0000, 9.0000, 12.0000, 9.0000]  (the centre of full)
valid = [6.0000, 9.0000, 12.0000]  (only where the kernel fits entirely)
```
### Example 3

A kernel longer than the input.

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
  // A kernel longer than the input is not an error: the two sequences trade roles and the lengths follow.
  const pluss::Series x = pluss::make_series({1.0, 2.0});
  const pluss::Series k = pluss::make_series({1.0, 1.0, 1.0});
  for (const auto [label, mode] : {std::pair{"full ", sig::ConvMode::Full}, {"same ", sig::ConvMode::Same}, {"valid", sig::ConvMode::Valid}}) {
    const pluss::Series y = sig::conv(x, k, -1, mode);
    out.writeln(label, "length", y.numel(), "values", y.items<double>());
  }
}
```

Output

```text
full  length 4 values [1.0000, 3.0000, 3.0000, 2.0000]
same  length 2 values [3.0000, 3.0000]
valid length 2 values [3.0000, 3.0000]
```
## See Also

See [Filtering and response](../filtering.md) for alignment, streaming-state, and axis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="PadMode.html">
    <span class="label">Next</span>
    <span class="title">PadMode</span>
  </a>
</div>
<!-- apiref:nav:end -->
