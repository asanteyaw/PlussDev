<!-- generated from synchronisation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Synchronisation

[Back to Communications 1: Modulation and Detection](../comms_01_modulation.md)

A receiver does not know when the symbols start or at what frequency and phase the carrier was sent. This page estimates the symbol timing of a shaped waveform and removes it, estimates a carrier frequency offset and removes it, and tracks the remaining phase with a phase-locked loop.

## The mental model

**Timing.** The squared magnitude of a pulse-shaped waveform has a tone at the symbol rate whose phase is the timing offset (the Oerder-Meyr estimator), whatever the symbols are. `timing` returns that offset in samples, modulo one symbol, and `delay` removes it by a fractional shift (a linear phase in the frequency domain, exact for a band-limited waveform and circular at the ends).

**Carrier.** For M-PSK symbols the M-th power removes the modulation and leaves a tone at M times the frequency offset. `cfo` finds its spectral peak (refined by parabolic interpolation) and returns the offset in cycles per symbol; `derot` multiplies symbol `n` by `exp(-j (2 pi f n + phi))` to remove it. `pll` then tracks the phase with a second-order loop (a Costas loop with an M-th power detector), leaving a phase ambiguity of a multiple of `2 pi / M` to be resolved with a preamble or differential coding.

## Timing

A waveform delayed by `d` samples gives an estimate of `d`; the program checks four delays, including a negative one.

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

The estimates are `0`, `1.370`, `2.900` and `-2.400` for the true delays. Removing the estimate restores the symbols: the error vector magnitude after matched filtering falls from `0.42` for the delayed waveform to `0.002` once the estimated delay is removed, and the estimate stays within `0.12` samples at `10 dB`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 400, 71));
  const pluss::Series wave = cm::shape(symbols, taps, 8);
  const pluss::Series late = cm::delay(wave, pluss::series::make(std::vector<double>{2.37}).select(0, 0));
  const auto evm_of = [&](const pluss::Series &w) { return cm::evm(cm::match(w, taps, 8, 380).slice(-1, 10, 370), symbols.slice(-1, 10, 370)).item<double>(); };
  const double estimate = cm::timing(late, 8).item<double>();
  out.writeln("estimated delay", estimate);
  out.writeln("evm sampling the delayed waveform as it is", evm_of(late));
  out.writeln("evm after removing the estimated delay   ", evm_of(cm::delay(late, pluss::series::make(std::vector<double>{-estimate}).select(0, 0))));
  // noisy: the estimate stays close at 10 dB Es/N0
  const pluss::Series noisy = cm::noise(late, 0.1, 5);
  out.writeln("estimated delay at 10 dB", cm::timing(noisy, 8).item<double>());
}
```

Output

```text
estimated delay 2.3700
evm sampling the delayed waveform as it is 0.4175
evm after removing the estimated delay    0.0020
estimated delay at 10 dB 2.2553
```

Each frame has its own timing, so the estimate and the delay take one value per frame.

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
  // one waveform per row, each with its own delay: delay and estimate take a vector of delays
  const cm::Constellation c = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series one = wave_of(c, taps, 3, 300);
  const pluss::Series frames = pluss::series::zeros({3, one.size(-1)}, pluss::Config{}.with_dtype(pluss::ScalarType::Complex128)).add(one);
  const pluss::Series delays = vec({0.5, 1.5, 3.2});
  const pluss::Series late = cm::delay(frames, delays);
  out.writeln("true delays     ", flat(delays));
  out.writeln("estimated delays", flat(cm::timing(late, 8)));
}
```

Output

```text
true delays      [0.500, 1.500, 3.200]
estimated delays [0.500, 1.500, 3.200]
```

`delay` is its own inverse for the opposite shift, and a whole-sample delay is a circular rotation.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series wave = wave_of(cm::qam(16), taps, 4, 200);
  // a delay is undone by the opposite delay; the shift is circular, so the waveform is exactly recovered
  const pluss::Series there = cm::delay(wave, pluss::series::make(std::vector<double>{3.4}).select(0, 0));
  out.writeln("delay by 3.4 and back, largest difference", cm::delay(there, pluss::series::make(std::vector<double>{-3.4}).select(0, 0)).sub(wave).abs().amax().item<double>());
  // a whole-sample delay is an exact circular shift
  const pluss::Series shifted = cm::delay(wave, pluss::series::make(std::vector<double>{5.0}).select(0, 0));
  out.writeln("delay by 5 samples equals a circular shift", shifted.slice(-1, 5, 20).sub(wave.slice(-1, 0, 15)).abs().amax().item<double>() < 1e-10);
  out.writeln("delay of a real signal stays real", !cm::delay(vec({0, 1, 0, 0, 0, 0, 0, 0}), pluss::series::make(std::vector<double>{2.0}).select(0, 0)).dtype().is_complex());
}
```

Output

```text
delay by 3.4 and back, largest difference 0.000000
delay by 5 samples equals a circular shift true
delay of a real signal stays real true
```

## Carrier frequency

QPSK with an offset of `0.0031` cycles per symbol and a phase of `0.4 rad` is recovered to the printed digits even at an noise variance of `0.2`, where the symbols are heavily noisy.

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

The estimate is `0.003100` at all three noise levels (`0.003101` at the highest), and `0.010000` for BPSK with its offset of `0.01`. `derot` applies and removes an offset; the angles of four identical symbols after derotating are `-0.3, -0.614, -0.928, -1.242`, the phase and then `2 pi f` more at every symbol.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  // derotation multiplies symbol n by exp(-j (2 pi f n + phi))
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series symbols = cm::symbols(qpsk, pluss::series::make(std::vector<std::int64_t>{0, 0, 0, 0}));
  const pluss::Series f = pluss::series::make(std::vector<double>{0.05}).select(0, 0), phi = pluss::series::make(std::vector<double>{0.3}).select(0, 0);
  const pluss::Series turned = cm::derot(symbols, f, phi);
  out.writeln("angles of four identical symbols after derotating", flat(turned.imag().atan2(turned.real())));
  out.writeln("expected", -0.3, -0.3 - 2.0 * M_PI * 0.05, -0.3 - 4.0 * M_PI * 0.05, -0.3 - 6.0 * M_PI * 0.05);
  // derotating by the opposite sign restores them: derot by (-f, -phi) rotates forward
  const pluss::Series back = cm::derot(turned, f.neg(), phi.neg());
  out.writeln("round trip difference", back.sub(symbols).abs().amax().item<double>());
}
```

Output

```text
angles of four identical symbols after derotating [-0.30000, -0.61416, -0.92832, -1.24248]
expected -0.30000 -0.61416 -0.92832 -1.24248
round trip difference 0.00000
```

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
  out.print_defaults(PrintOptions{}.fixed(5));
  // one offset per frame
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(3 * 2 * 500, 5).reshape({3, 1000}));
  const pluss::Series offsets = vec({0.001, 0.002, -0.003});
  const pluss::Series rotated = cm::derot(data, offsets.neg(), vec({0.0, 0.0, 0.0}));
  out.writeln("true offsets     ", flat(offsets));
  out.writeln("estimated offsets", flat(cm::cfo(rotated, 4)));
  out.writeln("restored by derotating with the estimates", cm::derot(rotated, cm::cfo(rotated, 4), vec({0.0, 0.0, 0.0})).sub(data).abs().amax().item<double>() < 0.02);
}
```

Output

```text
true offsets      [0.00100, 0.00200, -0.00300]
estimated offsets [0.00100, 0.00200, -0.00300]
restored by derotating with the estimates true
```

## Tracking the phase

The estimate removes most of the offset but not drift, and a first estimate is not exact. `pll` tracks the phase symbol by symbol. The program recovers QPSK with an offset of `0.003` cycles per symbol and a phase of `0.4 rad` at `20 dB`.

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

The tracked phase follows the true one, `0.4 + 2 pi 0.003 n`, to within a few hundredths of a radian from the first symbols to symbol `2999` (`56.91` against `56.93`), and the locked symbols sit on the constellation with an error vector magnitude of `0.10`, the noise level.

The loop bandwidth trades the speed of lock and the offset it can track against the noise it passes.

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

With a bandwidth of `0.005` the loop has not settled in `2000` symbols (error vector magnitude `0.34`); at `0.02` and `0.08` it is at the noise floor of `0.20`, the wider loop slightly noisier.

## Timing recovery with a loop

`timing` is one estimate for a block. A **Gardner loop** recovers the timing continuously, and follows a drifting sample clock: `matched` filters the waveform without sampling it, and `gardner` steers an interpolated sampling instant by the Gardner error `Re{conj(y(mid)) (y(n) - y(n - 1))}`, which is positive when the instant is late and needs neither the symbols nor the carrier phase. `matched` is also the first half of `match`.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 10);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 50, 4));
  const pluss::Series wave = cm::shape(symbols, taps, 8);
  // the matched filter is a convolution with the reversed taps: its length is the waveform plus the taps minus one
  const pluss::Series filtered = cm::matched(wave, taps);
  out.writeln("waveform", wave.sizes(), " taps", taps.numel(), " filtered", filtered.sizes());
  // sampling it at the combined delay (taps - 1) and every symbol period is what match does
  const std::int64_t delay = taps.numel() - 1;
  out.writeln("sampled filtered output equals match", filtered.select(-1, delay).sub(cm::match(wave, taps, 8, 50).select(-1, 0)).abs().item<double>() < 1e-12);
  out.writeln("peak of the filtered pulse of the first symbol is at the delay", flat(filtered.abs().slice(-1, delay - 2, delay + 3)));
  // the filtered waveform of a batch keeps its frames
  const pluss::Series frames = pluss::series::zeros({2, wave.size(-1)}, pluss::Config{}.with_dtype(pluss::ScalarType::Complex128)).add(wave);
  out.writeln("batched filtered shape", cm::matched(frames, taps).sizes());
}
```

Output

```text
waveform [473]  taps 81  filtered [553]
sampled filtered output equals match true
peak of the filtered pulse of the first symbol is at the delay [0.99975, 1.04001, 1.00088, 0.88084, 0.68848]
batched filtered shape [2, 553]
```

The filtered waveform is `81 - 1 = 80` samples longer than the shaped waveform, and the pulse of the first symbol peaks at the `taps - 1` delay.

The program feeds three waveforms with different timing offsets to the loop and finds, for each, the alignment of the recovered stream with the transmitted one that gives the smallest error vector magnitude.

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
static pluss::Series chain(const pluss::Series &symbols, const pluss::Series &taps, double delay_samples) {
  return cm::matched(cm::delay(cm::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{delay_samples}).select(0, 0)), taps);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series symbols = cm::modulate(qpsk, random_bits(2 * 800, 91));
  // three unknown timing offsets: the loop finds the sampling instants by itself
  out.writeln("true delay  recovered symbols  best evm over the stream alignment");
  for (double delay : {0.0, 1.7, -1.1}) {
    const cm::TimingResult got = cm::gardner(chain(symbols, taps, delay), 4, 0.02);
    double best = 1e9;
    for (int lag = 0; lag < 30; ++lag) best = std::min(best, cm::evm(got.symbols.slice(-1, 200 + lag, 700 + lag), symbols.slice(-1, 200, 700)).item<double>());
    out.writeln(delay, got.symbols.sizes(), best);
  }
  // the sampling instants advance by one symbol (4 samples) per symbol once the loop has settled
  const cm::TimingResult one = cm::gardner(chain(symbols, taps, 1.7), 4, 0.02);
  const std::vector<double> instants = flat(one.offset);
  out.writeln("sampling instants of symbols 600 to 604", instants[600], instants[601], instants[602], instants[603], instants[604]);
  out.writeln("offsets shape", one.offset.sizes(), " symbols shape", one.symbols.sizes());
}
```

Output

```text
true delay  recovered symbols  best evm over the stream alignment
0.0000 [820] 0.0300
1.7000 [820] 0.0300
-1.1000 [820] 0.0298
sampling instants of symbols 600 to 604 2405.6713 2409.6291 2413.6382 2417.6676 2421.7056
offsets shape [820]  symbols shape [820]
```

The loop locks at every offset: `0.030` error vector magnitude for `0`, `1.7` and `-1.1` samples, and the sampling instants advance by almost exactly four samples (one symbol) per symbol once settled (`2405.67`, `2409.63`, `2413.64`, `2417.67`, `2421.71`). The recovered stream is a shifted copy of the sent one by an unknown whole number of symbols; align it with a preamble.

The loop bandwidth trades settling against noise, and too wide a loop loses lock.

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
static pluss::Series chain(const pluss::Series &symbols, const pluss::Series &taps, double delay_samples) {
  return cm::matched(cm::delay(cm::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{delay_samples}).select(0, 0)), taps);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // a wider loop settles faster and tracks a larger timing drift but passes more noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series symbols = cm::modulate(qpsk, random_bits(2 * 1000, 7));
  const pluss::Series noisy = cm::noise(chain(symbols, taps, 1.3), 0.02, 9);
  out.writeln("loop bandwidth  best evm over the stream alignment after settling");
  for (double bandwidth : {0.005, 0.01, 0.03, 0.1}) {
    const cm::TimingResult got = cm::gardner(noisy, 4, bandwidth);
    double best = 1e9;
    for (int lag = 0; lag < 40; ++lag) best = std::min(best, cm::evm(got.symbols.slice(-1, 400 + lag, 900 + lag), symbols.slice(-1, 400, 900)).item<double>());
    out.writeln(bandwidth, best);
  }
}
```

Output

```text
loop bandwidth  best evm over the stream alignment after settling
0.0050 0.1196
0.0100 0.1205
0.0300 0.1297
0.1000 1.1952
```

At `20 dB` the error vector magnitude is `0.12` for bandwidths `0.005` and `0.01`, `0.13` at `0.03`, and the loop at `0.1` has lost lock (`1.2`).

## Errors

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series wave = wave_of(cm::psk(4), taps, 1, 100);
  report([&] { (void)cm::timing(wave, 2); });
  report([&] { (void)cm::timing(wave.slice(-1, 0, 20), 8); });
  report([&] { (void)cm::delay(wave, vec({std::nan("")}).select(0, 0)); });
  report([&] { (void)cm::delay(pluss::Series{}, vec({1.0}).select(0, 0)); });
}
```

Output

```text
eng::comms::timing: samples_per_symbol must be at least 4
eng::comms::timing: waveform must hold at least four symbols per frame
eng::comms::delay: samples must be finite real values
eng::comms::delay: signal must be a real or complex [..., L] array
```

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
  report([&] { (void)cm::cfo(data, 3); });
  report([&] { (void)cm::cfo(data.slice(-1, 0, 8), 4); });
  report([&] { (void)cm::cfo(random_bits(200, 1), 4); });
}
```

Output

```text
eng::comms::cfo: order must be 2, 4 or 8
eng::comms::cfo: symbols must be a complex [..., N] array with N >= 16
eng::comms::cfo: symbols must be a complex [..., N] array with N >= 16
```

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
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(40, 1));
  const pluss::Series zero = pluss::series::make(std::vector<double>{0.0}).select(0, 0);
  report([&] { (void)cm::derot(random_bits(40, 1), zero, zero); });
  report([&] { (void)cm::derot(pluss::Series{}, zero, zero); });
}
```

Output

```text
eng::comms::derot: symbols must be a complex [..., N] array
eng::comms::derot: symbols must be a complex [..., N] array
```

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

## Decisions

- **Timing first:** for a block with one fixed offset estimate on the waveform before the matched filter, remove it with `delay`, then `match`; for a drifting clock use `matched` and the Gardner loop `gardner` with a bandwidth of `0.005` to `0.03`.
- **Large frequency offsets:** `cfo` then `derot`, then `pll` for the residual; the estimator's range is `1 / (2 M)` cycles per symbol.
- **Loop bandwidth:** `0.01` to `0.03` of the symbol rate balances lock and noise; narrower for clean channels with small offsets.
- **The phase ambiguity:** resolve a multiple of `2 pi / M` with a known preamble or differential coding.

## Where next

Reference pages: [`timing`](../../../../api/eng/comms/synchronisation/timing.md), [`delay`](../../../../api/eng/comms/synchronisation/delay.md), [`cfo`](../../../../api/eng/comms/synchronisation/cfo.md), [`derot`](../../../../api/eng/comms/synchronisation/derot.md), [`pll`](../../../../api/eng/comms/synchronisation/pll.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iterative_codes.html">
    <span class="label">Previous</span>
    <span class="title">LDPC and turbo codes</span>
  </a>
</div>
<!-- apiref:nav:end -->
