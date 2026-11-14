<!-- generated from synchronisation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Synchronisation

This branch recovers timing and carrier. `timing` and `delay` find and remove a symbol-timing offset, `cfo` and `derot` find and remove a carrier offset, `pll` tracks the phase with a phase-locked loop, and `matched` and `gardner` recover the symbol timing continuously with a Gardner loop.

## Choosing A Surface

Estimate and remove timing on the waveform before `match`; estimate and remove the carrier offset on the symbols after it, then let `pll` track what is left.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Timing estimates.

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
static pluss::Series wave_of(const cm::Constellation &c, const pluss::Series &taps, std::uint64_t seed, std::int64_t symbols) {
  return cm::shape(cm::modulate(c, random_bits(c.bits * symbols, seed)), taps, 8);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series wave = wave_of(c, taps, 71, 400);
  // a waveform delayed by d samples gives an estimate of d (modulo a symbol, here 8 samples)
  out.writeln("   true    estimate");
  for (double d : {0.0, 1.37, 2.9, -2.4}) {
    const pluss::Series late = cm::delay(wave, pluss::series::make(std::vector<double>{d}).select(0, 0));
    out.writeln(d, cm::timing(late, 8).item<double>());
  }
}
```

Output

```text
   true    estimate
0.000 -0.000
1.370 1.370
2.900 2.900
-2.400 -2.400
```
### Example 2

Frequency offset estimates.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  // QPSK symbols with a carrier offset of 0.0031 cycles per symbol and a phase of 0.4 rad, with a little noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 3000, 81));
  const pluss::Series zero = pluss::series::make(std::vector<double>{0.0}).select(0, 0);
  const pluss::Series rotated = cm::derot(data, pluss::series::make(std::vector<double>{-0.0031}).select(0, 0), pluss::series::make(std::vector<double>{-0.4}).select(0, 0));
  for (double n0 : {0.0, 0.05, 0.2}) {
    const pluss::Series received = n0 > 0.0 ? cm::noise(rotated, n0, 3) : rotated;
    out.writeln("noise variance", n0, " estimated offset", cm::cfo(received, 4).item<double>(), " true 0.003100");
  }
  // BPSK and 8-PSK use the matching power
  out.writeln("BPSK estimate", cm::cfo(cm::derot(cm::modulate(cm::psk(2), random_bits(3000, 4)), pluss::series::make(std::vector<double>{-0.01}).select(0, 0), zero), 2).item<double>());
}
```

Output

```text
noise variance 0.000000  estimated offset 0.003100  true 0.003100
noise variance 0.050000  estimated offset 0.003100  true 0.003100
noise variance 0.200000  estimated offset 0.003101  true 0.003100
BPSK estimate 0.010000
```
### Example 3

A locked carrier loop.

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
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

synchronisation/timing
synchronisation/delay
synchronisation/cfo
synchronisation/derot
synchronisation/CarrierResult
synchronisation/pll
synchronisation/matched
synchronisation/TimingResult
synchronisation/gardner
```
<!-- apiref:toctree:end -->
