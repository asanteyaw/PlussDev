<!-- generated from pulse_shaping.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pulse shaping

[Back to Communications 1: Modulation and Detection](../comms_01_modulation.md)

Symbols are numbers; a transmitter sends a waveform. Putting symbols straight onto a waveform as short pulses needs unlimited bandwidth, so they are shaped into a smooth pulse that fits a band, and the receiver filters with the matching pulse before sampling. This page designs a root-raised-cosine pulse, shapes a stream, recovers the symbols with the matched filter, and checks the whole chain, with noise, against theory.

## The mental model

`rrc(rolloff, samples_per_symbol, span)` returns the taps of a root-raised-cosine pulse, `span * samples_per_symbol + 1` of them, scaled to unit energy. Used at both ends, transmitter and receiver, the pair makes a **raised-cosine** response that is exactly zero at every other symbol instant: no intersymbol interference. `shape(symbols, taps, samples_per_symbol)` places each symbol every `samples_per_symbol` samples and filters with the taps; `match(waveform, taps, samples_per_symbol, count)` filters with the time-reversed taps and keeps `count` samples at the symbol instants, which start after the combined delay of the two filters, `taps - 1` samples.

The `rolloff` between `0` and `1` is the excess bandwidth: `0` is the narrowest pulse and decays slowest; `1` is the widest. Unit-energy taps keep white noise at its per-sample variance through the matched filter, so the noise variance of a symbol after matching is the `N0` that was added per sample.

## The pulse

A short pulse shows the shape: `25` taps, symmetric about a centre tap, unit energy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const pluss::Series taps = cm::rrc(0.35, 4, 6);
  out.writeln("taps", taps.numel(), " (span 6 symbols x 4 samples + 1)");
  out.writeln("energy", taps.mul(taps).sum().item<double>());
  out.writeln("centre tap", flat(taps)[12], " first and last", flat(taps)[0], flat(taps)[24]);
  out.writeln("symmetric", taps.sub(taps.flip({0})).abs().amax().item<double>() < 1e-14);
}
```

Output

```text
taps 25  (span 6 symbols x 4 samples + 1)
energy 1.00000
centre tap 0.54795  first and last -0.01273 -0.01273
symmetric true
```

The energy is exactly one and the pulse is symmetric. The taps may be single precision.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7));
  const pluss::Series taps = cm::rrc(0.35, 4, 6, pluss::ScalarType::Float32);
  out.writeln("single precision taps", taps.scalar_type() == pluss::ScalarType::Float32);
  out.writeln("energy", taps.to(pluss::ScalarType::Float64).mul(taps.to(pluss::ScalarType::Float64)).sum().item<double>());
}
```

Output

```text
single precision taps true
energy 1.0000000
```

## No intersymbol interference

Convolving the pulse with itself gives the response from transmitted symbol to the sample of the receiver. It should be one at the symbol instant and zero at the others. A truncated pulse leaves a small residue that falls with the span. The program prints the largest residue at the other symbol instants for four rolloffs, at a span of `16` symbols.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // the pulse paired with itself must be zero at every other symbol instant (no intersymbol interference)
  for (double rolloff : {0.0, 0.25, 0.5, 1.0}) {
    const pluss::Series taps = cm::rrc(rolloff, 8, 16);
    const pluss::Series both = pluss::subsp::eng::signal::conv(taps, taps);
    const std::vector<double> r = flat(both);
    const std::size_t mid = r.size() / 2;
    double worst = 0.0;
    for (int k = 1; k <= 8; ++k) worst = std::max(worst, std::abs(r[mid + 8 * static_cast<std::size_t>(k)]));
    out.writeln("rolloff", rolloff, " peak", r[mid], " largest interference at symbol instants", worst);
  }
}
```

Output

```text
rolloff 0.00000  peak 1.00000  largest interference at symbol instants 0.05733
rolloff 0.25000  peak 1.00000  largest interference at symbol instants 0.00047
rolloff 0.50000  peak 1.00000  largest interference at symbol instants 0.00010
rolloff 1.00000  peak 1.00000  largest interference at symbol instants 0.00012
```

The peak is one for every rolloff. The interference is `0.057` for a rolloff of zero (a truncated sinc decays slowly), `4.7e-4` at `0.25`, `1.0e-4` at `0.5` and `1.2e-4` at `1`: a rolloff of a quarter or more makes the truncation error negligible at this span.

## Shaping and matching

The program shapes `100` 16-QAM symbols at eight samples per symbol and recovers them with the matched filter. In a noise-free chain the symbols come back to within the residue above.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::psk(4);
  const pluss::Series symbols = cm::modulate(c, random_bits(2 * 16, 4));
  const pluss::Series taps = cm::rrc(0.35, 8, 10);
  const pluss::Series waveform = cm::shape(symbols, taps, 8);
  out.writeln("symbols", symbols.sizes(), " taps", taps.numel(), " waveform", waveform.sizes(), " = (16 - 1) * 8 + 81");
  out.writeln("waveform is complex", waveform.dtype().is_complex());
  const double peak = waveform.abs().amax().item<double>();
  out.writeln("peak amplitude", peak);
}
```

Output

```text
symbols [16]  taps 81  waveform [201]  = (16 - 1) * 8 + 81
waveform is complex true
peak amplitude 0.4407
```

Sixteen symbols at eight samples each with an `81`-tap pulse make a waveform of `(16 - 1) * 8 + 81 = 201` samples.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // shape then match: the symbols come back, up to the small interference of a truncated pulse
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(4 * 100, 8);
  const pluss::Series sent = cm::modulate(c, bits);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);
  const pluss::Series received = cm::match(cm::shape(sent, taps, 8), taps, 8, 100);
  out.writeln("received symbols", received.sizes());
  out.writeln("error vector magnitude", cm::evm(received, sent).item<double>());
  out.writeln("every bit recovered", cm::ber(bits, cm::demod(c, received)).item<double>() == 0.0);
}
```

Output

```text
received symbols [100]
error vector magnitude 0.0020
every bit recovered true
```

The error vector magnitude is `0.002` and every bit is recovered.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.25, 4, 8);
  const pluss::Series symbols = cm::modulate(c, random_bits(2 * 4 * 20, 5).reshape({2, 80}));
  const pluss::Series waveform = cm::shape(symbols, taps, 4);
  out.writeln("symbols", symbols.sizes(), " waveform", waveform.sizes(), " (frame, sample)");
  // a real pulse train: BPSK as real symbols
  const pluss::Series real_symbols = vec({1, -1, 1, 1, -1});
  out.writeln("real symbols give a real waveform", cm::shape(real_symbols, taps, 4).dtype().is_complex() ? "complex" : "real", cm::shape(real_symbols, taps, 4).sizes());
}
```

Output

```text
symbols [2, 20]  waveform [2, 109]  (frame, sample)
real symbols give a real waveform real [49]
```

## Noise through the chain

The program shapes `20000` QPSK symbols, adds white noise of the variance that gives `Eb/N0 = 6 dB` at every waveform sample, matches, and compares the noise that reaches the decisions and the error rate with theory.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // unit-energy taps: white noise of variance N0 per waveform sample is still variance N0 per symbol after matching
  const cm::Constellation c = cm::psk(4);
  const pluss::Series bits = random_bits(2 * 20000, 3);
  const pluss::Series sent = cm::modulate(c, bits);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const double n0 = cm::n0(6.0, c);
  const pluss::Series waveform = cm::noise(cm::shape(sent, taps, 4), n0, 21);
  const pluss::Series received = cm::match(waveform, taps, 4, 20000);
  const pluss::Series noise = received.sub(sent);
  out.writeln("noise variance after matching", noise.abs().mul(noise.abs()).mean().item<double>(), " target", n0);
  out.writeln("measured bit error rate", cm::ber(bits, cm::demod(c, received)).item<double>(),
              " theory", cm::theory(c, vec({6.0})).items<double>()[0]);
}
```

Output

```text
noise variance after matching 0.12503  target 0.12559
measured bit error rate 0.00260  theory 0.00239
```

The noise variance after matching is `0.1250`, the target `0.1256`: the unit-energy matched filter leaves the white noise at its per-sample variance. The measured bit error rate is `0.0026` against `0.0024` by theory, so the shaped and matched chain performs as the theoretical one does.

## Errors

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  report([&] { (void)cm::rrc(1.5, 8, 10); });
  report([&] { (void)cm::rrc(-0.1, 8, 10); });
  report([&] { (void)cm::rrc(0.35, 1, 10); });
  report([&] { (void)cm::rrc(0.35, 8, 1); });
  report([&] { (void)cm::rrc(0.35, 8, 10, pluss::ScalarType::Int64); });
}
```

Output

```text
eng::comms::rrc: rolloff must lie in [0, 1]
eng::comms::rrc: rolloff must lie in [0, 1]
eng::comms::rrc: samples_per_symbol must be at least 2
eng::comms::rrc: span must be at least 2 symbols
eng::comms::rrc: dtype must be Float64 or Float32
```

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series taps = cm::rrc(0.35, 4, 6);
  const pluss::Series symbols = cm::modulate(cm::psk(4), random_bits(8, 1));
  report([&] { (void)cm::shape(symbols, taps, 0); });
  report([&] { (void)cm::shape(symbols, taps.reshape({5, 5}), 4); });
  report([&] { (void)cm::shape(pluss::Series{}, taps, 4); });
}
```

Output

```text
eng::comms::shape: samples_per_symbol must be at least 1
eng::comms::shape: taps must be a real vector
eng::comms::shape: symbols must be a [..., N] array
```

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> labels_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series taps = cm::rrc(0.35, 4, 6);
  const pluss::Series waveform = cm::shape(cm::modulate(cm::psk(4), random_bits(8, 1)), taps, 4);
  report([&] { (void)cm::match(waveform, taps, 4, 50); });
  report([&] { (void)cm::match(waveform, taps, 0, 4); });
  report([&] { (void)cm::match(waveform, taps, 4, 0); });
  report([&] { (void)cm::match(waveform, vec({}), 4, 4); });
}
```

Output

```text
eng::comms::match: the waveform is too short for the requested number of symbols
eng::comms::match: samples_per_symbol and count must be at least 1
eng::comms::match: samples_per_symbol and count must be at least 1
eng::comms::match: taps must be a real vector
```

## Decisions

- **Both ends:** `rrc` with the same rolloff and span at the transmitter and the receiver.
- **Rolloff:** `0.2` to `0.35` for a narrow band; larger when bandwidth is plentiful and timing errors matter.
- **Span:** `8` to `16` symbols; the residue of the truncation falls with it.
- **Oversampling:** at least `4` samples per symbol so the band fits below the Nyquist rate.

## Pitfalls

The program makes three mistakes: matching with a different pulse than the one that shaped, asking for more symbols than the waveform holds, and expecting a rolloff of zero to behave like the others.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(4 * 100, 8);
  const pluss::Series sent = cm::modulate(c, bits);
  const pluss::Series taps = cm::rrc(0.35, 8, 12);

  // 1. a receiver pulse with another rolloff leaves interference
  const pluss::Series waveform = cm::shape(sent, taps, 8);
  out.writeln("evm, matching with the same pulse         ", cm::evm(cm::match(waveform, taps, 8, 100), sent).item<double>());
  out.writeln("evm, matching with a rolloff of 0.9       ", cm::evm(cm::match(waveform, cm::rrc(0.9, 8, 12), 8, 100), sent).item<double>());

  // 2. more symbols than the waveform holds
  try {
    (void)cm::match(waveform, taps, 8, 200);
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }

  // 3. a rolloff of zero is a truncated sinc: the interference is far larger at a short span
  out.writeln("evm, rolloff 0 at a span of 12", cm::evm(cm::match(cm::shape(sent, cm::rrc(0.0, 8, 12), 8), cm::rrc(0.0, 8, 12), 8, 100), sent).item<double>());
}
```

Output

```text
evm, matching with the same pulse          0.0020
evm, matching with a rolloff of 0.9        0.0464
eng::comms::match: the waveform is too short for the requested number of symbols
evm, rolloff 0 at a span of 12 0.1448
```

Matching with the transmit pulse gives an error vector magnitude of `0.002`; matching with a rolloff of `0.9` against a transmit rolloff of `0.35` gives `0.046`, twenty times worse. Asking for `200` symbols from a waveform of `100` is refused with a message. And a rolloff of zero, a truncated sinc, leaves an error vector magnitude of `0.145` at a span of twelve symbols, where a rolloff of `0.35` leaves `0.002`.

## Where next

The reference pages cover every function of the chain: [`rrc`](../../../../api/eng/comms/pulse/rrc.md), [`shape`](../../../../api/eng/comms/pulse/shape.md), [`match`](../../../../api/eng/comms/pulse/match.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="noise_and_detection.html">
    <span class="label">Previous</span>
    <span class="title">Noise and detection</span>
  </a>
  <a class="next-link" href="ofdm.html">
    <span class="label">Next</span>
    <span class="title">OFDM and multipath</span>
  </a>
</div>
<!-- apiref:nav:end -->
