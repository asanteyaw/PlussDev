<!-- generated from coding.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Channel coding

This branch adds redundancy that corrects errors. `ConvolutionalCode` describes a feed-forward code, `conv_encode` encodes with termination and `viterbi_decode` decodes with soft or hard values. `ldpc_regular`, `ldpc_encode` and `ldpc_decode` give Gallager low-density parity-check codes with min-sum decoding, and `turbo_interleaver`, `turbo_encode` and `turbo_decode` give turbo codes with iterative max-log-MAP decoding.

## Choosing A Surface

Encode before `modulate`, decode after `llr`: the log-likelihood ratios of the demapper are the soft input of `viterbi_decode`.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

A codeword by hand.

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

Coded against uncoded BPSK.

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
### Example 3

Soft against hard decoding.

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
  // soft values (log-likelihood ratios) decode better than hard decisions of the same received symbols
  const cm::ConvolutionalCode code;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(20000, 15);
  const double n0 = cm::noise_variance(3.0, bpsk) / 0.5;
  const pluss::Series received = cm::awgn(cm::modulate(bpsk, cm::conv_encode(code, bits)), n0, 16);
  const pluss::Series soft = cm::viterbi_decode(code, cm::llr(bpsk, received, n0));
  const pluss::Series hard_bits = cm::demodulate(bpsk, received);
  const pluss::Series hard = cm::viterbi_decode(code, pluss::series::make(std::vector<double>{1.0}).add(hard_bits.to(pluss::ScalarType::Float64).mul(-2.0)));
  out.writeln("bit error rate with soft decisions", cm::bit_error_rate(bits, soft).item<double>(), " with hard decisions", cm::bit_error_rate(bits, hard).item<double>());
}
```

Output

```text
bit error rate with soft decisions 0.00075  with hard decisions 0.03490
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

coding/ConvolutionalCode
coding/encode
coding/decode
coding/LdpcCode
coding/gallager
coding/LdpcResult
coding/TurboCode
coding/shuffle
```
<!-- apiref:toctree:end -->
