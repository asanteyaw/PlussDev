<!-- generated from match.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::match`

Matched filtering and symbol-rate sampling.

## Signature

```cpp
pluss::Series comms::match(const pluss::Series& waveform, const pluss::Series& taps, std::int64_t samples_per_symbol, std::int64_t count);
```

## What It Does

Filters `waveform` with the time-reversed taps (the matched filter, also available unsampled as `matched`) and keeps `count` samples at the symbol instants, starting at the combined delay of the two pulses, `taps - 1` samples. With the transmit pulse of `shape` it recovers the symbols when the pair is a Nyquist pulse such as `rrc`.

## Parameters

`waveform`
: The received waveform, `[..., samples]`.

`taps`
: The same pulse the transmitter used.

`samples_per_symbol`
: The oversampling factor.

`count`
: The number of symbols to recover.

## Returns

A `[..., count]` array of symbol estimates.

## Errors

Throws `pluss::Error` for `samples_per_symbol` or `count` below `1`, taps that are not a real vector, or a waveform too short for `count` symbols.

## Interpretation and Use

The matched filter maximises the signal-to-noise ratio at the sampling instants, and with unit-energy taps white noise keeps its variance. Timing is assumed known: the first symbol is at the combined filter delay.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Shape then match.

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
### Example 2

Noise through the chain against theory.

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
### Example 3

Rejected arguments.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
</div>
<!-- apiref:nav:end -->
