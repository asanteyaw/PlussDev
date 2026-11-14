<!-- generated from modulate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::modulate`

Bits to symbols.

## Signature

```cpp
pluss::Series comms::modulate(const comms::Constellation& constellation, const pluss::Series& bits);
```

## What It Does

Groups the bits `bits` at a time (the first bit of a group is the most significant), reads each group as a label, and returns the label's point.

## Parameters

`constellation`
: The constellation to map to.

`bits`
: Zeros and ones, a real `[..., N]` array whose last length is a multiple of `constellation.bits`. Leading axes are independent frames.

## Returns

A complex `[..., N / bits]` array of symbols, of the constellation's dtype.

## Errors

Throws `pluss::Error` for a constellation not made by `psk` or `qam`, bits that are not a real array of zeros and ones, or a bit count that is not a multiple of the bits per symbol.

## Interpretation and Use

This is the transmitter side of the chain `modulate`, `shape`, `noise`, `match`, `demod`. Any real dtype of bits is accepted.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

QPSK and 16-QAM mappings.

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

Frames in a batch.

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
  // a leading axis is a batch of independent frames
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(3 * 80, 2).reshape({3, 80});
  const pluss::Series symbols = cm::modulate(c, bits);
  out.writeln("bits", bits.sizes(), " symbols", symbols.sizes(), " (frame, symbol)");
  out.writeln("round trip recovers every bit", cm::ber(bits, cm::demod(c, symbols)).amax().item<double>() == 0.0);
}
```

Output

```text
bits [3, 80]  symbols [3, 20]  (frame, symbol)
round trip recovers every bit true
```
### Example 3

Rejected bit arrays.

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
  const cm::Constellation c = cm::qam(16);
  report([&] { (void)cm::modulate(c, random_bits(7, 1)); });
  report([&] { (void)cm::modulate(c, vec({0, 1, 2, 1})); });
  report([&] { (void)cm::modulate(c, pluss::Series{}); });
  cm::Constellation broken;
  report([&] { (void)cm::modulate(broken, random_bits(8, 1)); });
}
```

Output

```text
eng::comms::to_labels: the number of bits must be a multiple of bits_per_symbol
eng::comms::to_labels: bits must be zeros and ones
eng::comms::to_labels: bits must be a real [..., N] array
eng::comms::modulate: the constellation is not one made by psk or qam
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="qam.html">
    <span class="label">Previous</span>
    <span class="title">qam</span>
  </a>
  <a class="next-link" href="symbols.html">
    <span class="label">Next</span>
    <span class="title">symbols</span>
  </a>
</div>
<!-- apiref:nav:end -->
