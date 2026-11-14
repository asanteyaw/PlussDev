<!-- generated from pulse.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pulse shaping

This branch turns symbols into a band-limited waveform and back. `rrc` designs the pulse, `shape` makes the waveform and `match` filters and samples it at the receiver.

## Choosing A Surface

Use `rrc` at both ends: the transmitter shapes with it and the receiver matches with it, and the pair has no intersymbol interference.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

The taps of a pulse.

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
### Example 2

A shaped waveform.

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
### Example 3

Shaping and matching.

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
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

pulse/rrc
pulse/shape
pulse/match
```
<!-- apiref:toctree:end -->
