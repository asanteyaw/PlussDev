<!-- generated from pll.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::pll`

A phase-locked loop for PSK carriers.

## Signature

```cpp
comms::CarrierResult comms::pll(const pluss::Series& symbols, std::int64_t order, double loop_bandwidth);
```

## What It Does

Runs a second-order phase-locked loop (a Costas loop with an M-th power detector) over the symbols: at every symbol the phase error is `arg(y^M) / M` of the corrected symbol `y`, and a proportional-plus-integral filter with damping `1 / sqrt(2)` updates the phase and the frequency. The loop runs over the symbols and is vectorized over the frames.

## Parameters

`symbols`
: Received PSK symbols at one sample per symbol, a complex `[..., N]` array.

`order`
: The PSK order: `2`, `4` or `8`.

`loop_bandwidth`
: The normalised noise bandwidth of the loop, in `(0, 0.5)`; about `0.005` to `0.05` of the symbol rate is typical.

## Returns

A `CarrierResult` with the corrected symbols and the tracked phase.

## Errors

Throws `pluss::Error` for an order other than `2`, `4` or `8`, a `loop_bandwidth` outside `(0, 0.5)`, or symbols that are not a complex array with at least two symbols.

## Interpretation and Use

A wider loop locks faster and tracks larger offsets but passes more noise. The lock leaves a phase ambiguity of a multiple of `2 pi / order`: resolve it with a known preamble or differential coding. Remove a large offset first with `cfo` and `derot`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Lock onto an offset carrier.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // QPSK with a carrier frequency offset of 0.003 cycles per symbol and a phase of 0.4 rad, 20 dB signal-to-noise ratio
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 3000, 81));
  const pluss::Series received = cm::noise(cm::derot(data, pluss::series::make(std::vector<double>{-0.003}).select(0, 0), pluss::series::make(std::vector<double>{-0.4}).select(0, 0)), 0.01, 5);
  const cm::CarrierResult locked = cm::pll(received, 4, 0.02);
  out.writeln("symbols", locked.symbols.sizes(), " phase", locked.phase.sizes());
  const std::vector<double> phase = flat(locked.phase);
  out.writeln("tracked phase at symbols 0, 500, 1000, 2000, 2999", phase[0], phase[500], phase[1000], phase[2000], phase[2999]);
  out.writeln("true phase at the same symbols", 0.4, 0.4 + 2 * M_PI * 0.003 * 500, 0.4 + 2 * M_PI * 0.003 * 1000, 0.4 + 2 * M_PI * 0.003 * 2000, 0.4 + 2 * M_PI * 0.003 * 2999);
  // once locked the symbols sit on the constellation up to a multiple of 90 degrees; resolve it against the first symbols
  const pluss::Series tail = locked.symbols.slice(-1, 1500, 3000);
  out.writeln("evm of the locked symbols against the nearest points", cm::evm(tail, cm::symbols(qpsk, cm::detect(qpsk, tail))).item<double>());
}
```

Output

```text
symbols [3000]  phase [3000]
tracked phase at symbols 0, 500, 1000, 2000, 2999 0.0000 9.8072 19.2509 38.1031 56.9105
true phase at the same symbols 0.4000 9.8248 19.2496 38.0991 56.9298
evm of the locked symbols against the nearest points 0.1013
```
### Example 2

Loop bandwidth against noise.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // a wider loop locks faster and tracks a larger offset but passes more noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 4000, 21));
  const pluss::Series received = cm::noise(cm::derot(data, pluss::series::make(std::vector<double>{-0.004}).select(0, 0), pluss::series::make(std::vector<double>{-1.0}).select(0, 0)), 0.04, 6);
  for (double bandwidth : {0.005, 0.02, 0.08}) {
    const cm::CarrierResult locked = cm::pll(received, 4, bandwidth);
    const pluss::Series settled = locked.symbols.slice(-1, 2000, 4000);
    out.writeln("loop bandwidth", bandwidth, " evm after settling", cm::evm(settled, cm::symbols(qpsk, cm::detect(qpsk, settled))).item<double>());
  }
}
```

Output

```text
loop bandwidth 0.0050  evm after settling 0.3428
loop bandwidth 0.0200  evm after settling 0.2015
loop bandwidth 0.0800  evm after settling 0.2076
```
### Example 3

Rejected arguments.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(200, 1));
  report([&] { (void)cm::pll(data, 3, 0.02); });
  report([&] { (void)cm::pll(data, 4, 0.0); });
  report([&] { (void)cm::pll(data, 4, 0.6); });
  report([&] { (void)cm::pll(random_bits(200, 1), 4, 0.02); });
}
```

Output

```text
eng::comms::pll: order must be 2, 4 or 8
eng::comms::pll: loop_bandwidth must lie in (0, 0.5)
eng::comms::pll: loop_bandwidth must lie in (0, 0.5)
eng::comms::pll: symbols must be a complex [..., N] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CarrierResult.html">
    <span class="label">Previous</span>
    <span class="title">CarrierResult</span>
  </a>
  <a class="next-link" href="matched.html">
    <span class="label">Next</span>
    <span class="title">matched</span>
  </a>
</div>
<!-- apiref:nav:end -->
