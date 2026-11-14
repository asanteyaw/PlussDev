<!-- generated from demod.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::demod`

Hard-decision demodulation to bits.

## Signature

```cpp
pluss::Series comms::demod(const comms::Constellation& constellation, const pluss::Series& received);
```

## What It Does

Detects the nearest point of every received symbol and expands its label to bits: `to_bits(detect(constellation, received), constellation.bits)`.

## Parameters

`constellation`
: The constellation the symbols were sent from.

`received`
: The received complex symbols, `[..., N]`.

## Returns

An `Int64` array of zeros and ones, `[..., N * bits]`.

## Errors

Throws `pluss::Error` for a constellation not made by `psk` or `qam` or an undefined or scalar `received`.

## Interpretation and Use

Use `llr` instead when a decoder follows: hard decisions discard how close each symbol was to the boundary.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series bits = random_bits(3 * 400, 6).reshape({3, 400});
  const pluss::Series received = cm::noise(cm::modulate(c, bits), 0.05, 8);
  const pluss::Series decided = cm::demod(c, received);
  out.writeln("bits", bits.sizes(), " decisions", decided.sizes(), " dtype is Int64", decided.scalar_type() == pluss::ScalarType::Int64);
  out.writeln("errors in each frame below 5", cm::ber(bits, decided).mul(400.0).items<double>());
}
```

Output

```text
bits [3, 400]  decisions [3, 400]  dtype is Int64 true
errors in each frame below 5 [0, 0, 0]
```
### Example 3

A full round trip.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="detect.html">
    <span class="label">Previous</span>
    <span class="title">detect</span>
  </a>
  <a class="next-link" href="llr.html">
    <span class="label">Next</span>
    <span class="title">llr</span>
  </a>
</div>
<!-- apiref:nav:end -->
