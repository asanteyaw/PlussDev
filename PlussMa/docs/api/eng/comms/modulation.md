<!-- generated from modulation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Modulation and detection

This branch maps bits to complex symbols and back. `psk` and `qam` build a `Constellation`, `modulate` and `symbols` map to symbols, `to_bits` and `to_labels` convert between bits and labels, `detect`, `demod` and `llr` decide what was sent, and `to_tones` and `from_tones` carry subcarrier symbols over a multipath channel with a cyclic prefix.

## Choosing A Surface

Use `psk` for phase modulation of any power-of-two order and `qam` for square amplitude-and-phase modulation; use `llr` rather than `demod` when a decoder will use the decisions.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

QPSK points by label.

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
  // QPSK: four points on the unit circle; the default labelling is Gray, so label 2 is at angle 3 pi / 2 and label 3 at angle pi
  const cm::Constellation qpsk = cm::psk(4);
  out.writeln("order", qpsk.order, " bits per symbol", qpsk.bits);
  out.writeln("real parts by label", flat(qpsk.points.real()));
  out.writeln("imaginary parts by label", flat(qpsk.points.imag()));
  const cm::Constellation eight = cm::psk(8);
  out.writeln("8-PSK point of label 5 (Gray)", flat(eight.points.real())[5], flat(eight.points.imag())[5]);
}
```

Output

```text
order 4  bits per symbol 2
real parts by label [1.0000, 0.0000, -0.0000, -1.0000]
imaginary parts by label [0.0000, 1.0000, -1.0000, 0.0000]
8-PSK point of label 5 (Gray) -0.0000 -1.0000
```
### Example 2

Bits to 16-QAM symbols.

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
### Example 3

Bit error rate against Eb/N0.

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
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(4 * 20000, 9);
  const pluss::Series symbols = cm::modulate(c, bits);
  for (double ebn0 : {4.0, 8.0, 12.0}) {
    const pluss::Series received = cm::noise(symbols, cm::n0(ebn0, c), 31);
    out.writeln("Eb/N0", ebn0, "dB  bit error rate", cm::ber(bits, cm::demod(c, received)).item<double>());
  }
}
```

Output

```text
Eb/N0 4.00000 dB  bit error rate 0.05809
Eb/N0 8.00000 dB  bit error rate 0.00976
Eb/N0 12.00000 dB  bit error rate 0.00021
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

modulation/Constellation
modulation/ConstellationOptions
modulation/psk
modulation/qam
modulation/modulate
modulation/symbols
modulation/to_bits
modulation/to_labels
modulation/detect
modulation/demod
modulation/llr
modulation/to_tones
modulation/from_tones
```
<!-- apiref:toctree:end -->
