<!-- generated from comms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Communications

Digital modulation and detection on complete blocks of data: PSK and square QAM with Gray labelling, hard and soft demodulation, root-raised-cosine pulse shaping with matched filtering, white Gaussian noise at a chosen Eb/N0, and the error metrics and theory to judge a link. Every call is vectorized over leading frame axes.

## Choosing A Surface

| Task | Branch |
|---|---|
| Constellations, mapping and detection | [Modulation and detection](comms/modulation.md) |
| Pulse shaping and matched filtering | [Pulse shaping](comms/pulse.md) |
| Noise at a given Eb/N0 | [Channel](comms/channel.md) |
| Bit error rate, EVM and theory | [Error metrics and theory](comms/metrics.md) |
| Convolutional coding and Viterbi decoding | [Channel coding](comms/coding.md) |
| Timing and carrier recovery | [Synchronisation](comms/synchronisation.md) |

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

A QPSK mapping.

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
  // QPSK: two bits per symbol, the first bit is the high bit of the label
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{0, 0, 0, 1, 1, 1, 1, 0});
  const pluss::Series symbols = cm::modulate(qpsk, bits);
  out.writeln("bits", labels_of(bits));
  out.writeln("symbols", symbols.sizes(), " real", flat(symbols.real()), " imag", flat(symbols.imag()));
  // 16-QAM: four bits per symbol
  out.writeln("16-QAM symbols from 8 bits", cm::modulate(cm::qam(16), pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1, 0, 0, 1, 0})).sizes());
}
```

Output

```text
bits [0, 0, 0, 1, 1, 1, 1, 0]
symbols [4]  real [1.0000, 0.0000, -1.0000, -0.0000]  imag [0.0000, 1.0000, 0.0000, -1.0000]
16-QAM symbols from 8 bits [2]
```
### Example 2

A link against theory.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  struct Row { cm::Constellation c; const char *name; double ebn0; };
  const std::vector<Row> rows = {{cm::psk(2), "BPSK  ", 4.0}, {cm::psk(4), "QPSK  ", 6.0}, {cm::qam(16), "16-QAM", 8.0}, {cm::qam(64), "64-QAM", 12.0}};
  out.writeln("             Eb/N0   simulated    theory");
  std::uint64_t seed = 1;
  for (const Row &row : rows) {
    const pluss::Series bits = random_bits(row.c.bits * 100000, seed++);
    const pluss::Series received = cm::noise(cm::modulate(row.c, bits), cm::n0(row.ebn0, row.c), seed++);
    out.writeln(row.name, row.ebn0, cm::ber(bits, cm::demod(row.c, received)).item<double>(), cm::theory(row.c, vec({row.ebn0})).items<double>()[0]);
  }
}
```

Output

```text
             Eb/N0   simulated    theory
BPSK   4.000000 0.012140 0.012501
QPSK   6.000000 0.002415 0.002388
16-QAM 8.000000 0.009338 0.009247
64-QAM 12.000000 0.009778 0.009724
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
:maxdepth: 3
:hidden:

comms/modulation
comms/pulse
comms/channel
comms/metrics
comms/coding
comms/synchronisation
```
<!-- apiref:toctree:end -->
