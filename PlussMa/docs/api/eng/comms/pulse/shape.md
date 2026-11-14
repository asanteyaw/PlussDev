<!-- generated from shape.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::shape`

Pulse shaping of a symbol stream.

## Signature

```cpp
pluss::Series comms::shape(const pluss::Series& symbols, const pluss::Series& taps, std::int64_t samples_per_symbol);
```

## What It Does

Places each symbol every `samples_per_symbol` samples, with zeros between, and filters the result with `taps`: the waveform that is actually transmitted.

## Parameters

`symbols`
: The symbols, a real or complex `[..., N]` array.

`taps`
: The pulse, a real vector, for example from `rrc`.

`samples_per_symbol`
: The oversampling factor, at least `1`.

## Returns

A `[..., (N - 1) * samples_per_symbol + taps]` waveform of the dtype of `symbols`.

## Errors

Throws `pluss::Error` for undefined or scalar `symbols`, taps that are not a real vector, or `samples_per_symbol` below `1`.

## Interpretation and Use

The waveform is longer than the symbol stream by the length of the pulse. Real symbols give a real waveform, complex symbols a complex one.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A shaped QPSK waveform.

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
### Example 2

A batch of frames, and real symbols.

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
### Example 4

Shaping then matching.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rrc.html">
    <span class="label">Previous</span>
    <span class="title">rrc</span>
  </a>
  <a class="next-link" href="match.html">
    <span class="label">Next</span>
    <span class="title">match</span>
  </a>
</div>
<!-- apiref:nav:end -->
