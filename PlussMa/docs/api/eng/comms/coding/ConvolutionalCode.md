<!-- generated from ConvolutionalCode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::ConvolutionalCode`

A feed-forward convolutional code.

## Signature

```cpp
comms::ConvolutionalCode
```

## What It Does

Describes a rate-`1 / n` convolutional code: each input bit produces `n` coded bits, one per generator, each the parity of the input bit and the previous `constraint_length - 1` bits selected by the generator. A generator's most significant of `constraint_length` bits multiplies the current input bit. The encoder is terminated with `constraint_length - 1` zero bits.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `constraint_length` | The number of input bits that influence an output, `K`, in `[2, 12]`. | `7` |
| `generators` | The generators as integers (written in octal), at least two, each non-zero and below `2^K`. | `{0171, 0133}` |

## Returns

A value; read by `conv_encode` and `viterbi_decode`.

## Errors

The functions throw `pluss::Error` for a constraint length outside `[2, 12]`, fewer than two generators, or a generator that is zero or does not fit in `K` bits.

## Interpretation and Use

The default is the standard rate-1/2, constraint-length-7 code (free distance 10). A longer constraint length corrects more at the cost of a decoder whose work grows as `2^(K - 1)`. The textbook code `{07, 05}` with `K = 3` is useful for hand checks.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The textbook code of constraint length 3.

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
  // the textbook rate-1/2 code of constraint length 3 with generators 7 and 5 (octal)
  cm::ConvolutionalCode code;
  code.constraint_length = 3;
  code.generators = {07, 05};
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1});
  const pluss::Series word = cm::conv_encode(code, bits);
  out.writeln("information bits", ints_of(bits));
  out.writeln("codeword (pairs, with the 2-bit zero tail)", ints_of(word), " length", word.sizes());
  out.writeln("the pairs are 11 10 00 01 01 11");
}
```

Output

```text
information bits [1, 0, 1, 1]
codeword (pairs, with the 2-bit zero tail) [1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1]  length [12]
the pairs are 11 10 00 01 01 11
```
### Example 2

The default code and a rate-1/3 code.

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
  // the default code is the rate-1/2 constraint-length-7 code with generators 171 and 133 (octal)
  const cm::ConvolutionalCode code;
  out.writeln("constraint length", code.constraint_length, " generators 0171 and 0133 octal, stored as", code.generators);
  const pluss::Series bits = random_bits(2 * 100, 3).reshape({2, 100});
  const pluss::Series word = cm::conv_encode(code, bits);
  out.writeln("bits", bits.sizes(), " codeword", word.sizes(), " = (100 + 6) x 2 per frame");
  // a rate-1/3 code from three generators
  cm::ConvolutionalCode third;
  third.constraint_length = 3;
  third.generators = {07, 07, 05};
  out.writeln("rate 1/3 codeword length for 100 bits", cm::conv_encode(third, random_bits(100, 1)).sizes());
}
```

Output

```text
constraint length 7  generators 0171 and 0133 octal, stored as [121, 91]
bits [2, 100]  codeword [2, 212]  = (100 + 6) x 2 per frame
rate 1/3 codeword length for 100 bits [306]
```
### Example 3

Rejected codes.

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
  const pluss::Series bits = random_bits(8, 1);
  report([&] { (void)cm::conv_encode(cm::ConvolutionalCode{1, {1, 1}}, bits); });
  report([&] { (void)cm::conv_encode(cm::ConvolutionalCode{3, {07}}, bits); });
  report([&] { (void)cm::conv_encode(cm::ConvolutionalCode{3, {07, 011}}, bits); });
  report([&] { (void)cm::conv_encode(cm::ConvolutionalCode{3, {07, 0}}, bits); });
  report([&] { (void)cm::conv_encode(cm::ConvolutionalCode{}, vec({0, 1, 2})); });
}
```

Output

```text
eng::comms::conv_encode: constraint_length must lie in [2, 12]
eng::comms::conv_encode: a code needs at least two generators
eng::comms::conv_encode: every generator must be non-zero and fit in constraint_length bits
eng::comms::conv_encode: every generator must be non-zero and fit in constraint_length bits
eng::comms::conv_encode: bits must be zeros and ones
```
### Example 4

Coding gain.

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
  // BPSK with the rate-1/2 code against uncoded BPSK at the same Eb/N0 (each coded bit carries half the energy per bit)
  const cm::ConvolutionalCode code;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(20000, 5);
  const pluss::Series sent = cm::conv_encode(code, bits);
  out.writeln("Eb/N0   uncoded     coded");
  for (double ebn0 : {2.0, 3.0, 4.0}) {
    const double n0_coded = cm::noise_variance(ebn0, bpsk) / 0.5;
    const pluss::Series received = cm::awgn(cm::modulate(bpsk, sent), n0_coded, 7);
    const pluss::Series decoded = cm::viterbi_decode(code, cm::llr(bpsk, received, n0_coded));
    const pluss::Series raw = cm::awgn(cm::modulate(bpsk, bits), cm::noise_variance(ebn0, bpsk), 8);
    out.writeln(ebn0, cm::bit_error_rate(bits, cm::demodulate(bpsk, raw)).item<double>(), cm::bit_error_rate(bits, decoded).item<double>());
  }
}
```

Output

```text
Eb/N0   uncoded     coded
2.00000 0.03595 0.00625
3.00000 0.02160 0.00075
4.00000 0.01150 0.00000
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>
<!-- apiref:nav:end -->
