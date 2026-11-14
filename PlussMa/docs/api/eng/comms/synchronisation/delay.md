<!-- generated from delay.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::delay`

A fractional delay of a waveform.

## Signature

```cpp
pluss::Series comms::delay(const pluss::Series& signal, const pluss::Series& samples);
```

## What It Does

Delays the signal by `samples` (real, fractional or negative) by multiplying its spectrum by a linear phase. The shift is circular, so a whole-sample delay is an exact rotation of the samples.

## Parameters

`signal`
: A real or complex `[..., L]` array with `L >= 2`.

`samples`
: The delay in samples: a scalar, or `[...]` with one delay per frame; finite.

## Returns

An array of the shape and kind of `signal`.

## Errors

Throws `pluss::Error` for a signal that is not a real or complex array, or a `samples` that is not finite real values.

## Interpretation and Use

Pad the ends of a waveform when the circular wrap-around matters. For a band-limited waveform sampled above the Nyquist rate the fractional shift is exact up to the wrap.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Delay and its inverse.

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
### Example 2

Removing an estimated delay.

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
### Example 3

A delay for each frame.

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
### Example 4

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timing.html">
    <span class="label">Previous</span>
    <span class="title">timing</span>
  </a>
  <a class="next-link" href="cfo.html">
    <span class="label">Next</span>
    <span class="title">cfo</span>
  </a>
</div>
<!-- apiref:nav:end -->
