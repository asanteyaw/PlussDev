<!-- generated from encode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::encode`

Encoding with a convolutional, LDPC or turbo code.

## Signature

```cpp
pluss::Series comms::encode(const comms::ConvolutionalCode& code, const pluss::Series& bits);

pluss::Series comms::encode(const comms::LdpcCode& code, const pluss::Series& bits);

pluss::Series comms::encode(const comms::TurboCode& code, const pluss::Series& interleaver, const pluss::Series& bits);
```

## What It Does

**Convolutional:** encodes the bits with every generator by convolution modulo two and interleaves the outputs per input bit, after appending `constraint_length - 1` zero tail bits so that the encoder ends in the zero state. **LDPC:** multiplies the information bits by the systematic generator modulo two: the codeword holds the information bits at `info_positions` and the parity bits elsewhere. **Turbo:** encodes the bits with the first recursive systematic encoder (terminated to the zero state with `K - 1` tail steps) and, in the interleaved order, with the second (left unterminated); the output is the systematic bits, the first parity stream, the second parity stream, and the tail of the first encoder (its systematic then parity bits).

## Parameters

`code`
: The code: a `ConvolutionalCode`, an `LdpcCode` or a `TurboCode`. The overload is chosen by its type.

`interleaver`
: Turbo only: a permutation of `0 .. N - 1`, as `shuffle` returns.

`bits`
: Zeros and ones, a real array whose leading axes are independent frames: `[..., N]` with `N >= 1` for the convolutional code, `[..., k]` with `k = code.dimension` for LDPC and `[..., N]` with `N >= 2` for turbo.

## Returns

An `Int64` array of zeros and ones: `[..., (N + constraint_length - 1) * generators]` (convolutional), `[..., n]` (LDPC) or `[..., 3 N + 2 (K - 1)]` (turbo).

## Errors

Throws `pluss::Error` for an invalid code, bits that are not a real array of zeros and ones, a last length that does not match the code, or (turbo) an interleaver that is not an integer permutation of the right length. The messages name the overload (`encode`) and the rule.

## Interpretation and Use

The convolutional tail costs `K - 1` extra coded symbols per frame, a small fraction of a long frame; the decoder uses it to start and end in a known state. Every LDPC codeword satisfies every parity check. Leaving the second turbo encoder unterminated costs a little performance at short blocks and keeps the layout simple. Modulate the result with `modulate`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A convolutional codeword checked by hand.

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
  const pluss::Series word = cm::encode(code, bits);
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

Lengths for two convolutional codes.

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
  const pluss::Series word = cm::encode(code, bits);
  out.writeln("bits", bits.sizes(), " codeword", word.sizes(), " = (100 + 6) x 2 per frame");
  // a rate-1/3 code from three generators
  cm::ConvolutionalCode third;
  third.constraint_length = 3;
  third.generators = {07, 07, 05};
  out.writeln("rate 1/3 codeword length for 100 bits", cm::encode(third, random_bits(100, 1)).sizes());
}
```

Output

```text
constraint length 7  generators 0171 and 0133 octal, stored as [121, 91]
bits [2, 100]  codeword [2, 212]  = (100 + 6) x 2 per frame
rate 1/3 codeword length for 100 bits [306]
```
### Example 3

Encoding a batch of LDPC frames.

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
static pluss::Series soft_of(const pluss::Series &codeword, double amplitude) {
  return pluss::series::make(std::vector<double>{1.0}).add(codeword.to(pluss::ScalarType::Float64).mul(-2.0)).mul(amplitude);
}

int main() {
  OutputEngine out;
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  const pluss::Series bits = random_bits(2 * code.dimension, 31).reshape({2, code.dimension});
  const pluss::Series word = cm::encode(code, bits);
  out.writeln("bits", bits.sizes(), " codewords", word.sizes(), " systematic: the information bits sit at the info positions", word.index_select(-1, pluss::series::make(std::vector<double>{static_cast<double>(code.info_positions[0]), static_cast<double>(code.info_positions[1])}).to(pluss::ScalarType::Int64)).sub(bits.slice(-1, 0, 2)).abs().amax().item<double>() == 0.0);
  const cm::LdpcResult decoded = cm::decode(code, soft_of(word, 5.0));
  out.writeln("noiseless: converged", decoded.converged.sizes(), "all true", decoded.converged.all().item<bool>(), " iterations", decoded.iterations.items<std::int64_t>()[0]);
  out.writeln("bits recovered", decoded.bits.sub(bits).abs().amax().item<double>() == 0.0, " codeword recovered", decoded.codeword.sub(word).abs().amax().item<double>() == 0.0);
}
```

Output

```text
bits [2, 602]  codewords [2, 1200]  systematic: the information bits sit at the info positions true
noiseless: converged [2] all true true  iterations 1
bits recovered true  codeword recovered true
```
### Example 4

The turbo codeword layout.

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
  // the (13, 15) turbo code of constraint length 4: systematic bits, two parity streams and the tail of the first encoder
  const cm::TurboCode code;
  const std::int64_t n = 12;
  const pluss::Series pi = cm::shuffle(n, 5);
  const pluss::Series bits = random_bits(n, 3);
  const pluss::Series word = cm::encode(code, pi, bits);
  out.writeln("interleaver", ints_of(pi));
  out.writeln("information bits", ints_of(bits));
  out.writeln("codeword length", word.sizes(), " = 3 x 12 + 2 x 3");
  out.writeln("systematic  ", ints_of(word.slice(-1, 0, n)));
  out.writeln("parity 1    ", ints_of(word.slice(-1, n, 2 * n)));
  out.writeln("parity 2    ", ints_of(word.slice(-1, 2 * n, 3 * n)));
  out.writeln("tail (systematic then parity)", ints_of(word.slice(-1, 3 * n, 3 * n + 6)));
  out.writeln("same seed, same interleaver", cm::shuffle(n, 5).sub(pi).abs().amax().item<std::int64_t>() == 0, " another seed", ints_of(cm::shuffle(n, 6)));
}
```

Output

```text
interleaver [1, 4, 8, 9, 6, 7, 0, 10, 11, 3, 5, 2]
information bits [0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0]
codeword length [42]  = 3 x 12 + 2 x 3
systematic   [0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0]
parity 1     [0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0]
parity 2     [0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1]
tail (systematic then parity) [0, 0, 0, 0, 0, 0]
same seed, same interleaver true  another seed [10, 9, 5, 0, 11, 1, 4, 6, 3, 7, 8, 2]
```
### Example 5

Rejected convolutional input.

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
  report([&] { (void)cm::encode(cm::ConvolutionalCode{1, {1, 1}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{3, {07}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{3, {07, 011}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{3, {07, 0}}, bits); });
  report([&] { (void)cm::encode(cm::ConvolutionalCode{}, vec({0, 1, 2})); });
}
```

Output

```text
eng::comms::encode: constraint_length must lie in [2, 12]
eng::comms::encode: a code needs at least two generators
eng::comms::encode: every generator must be non-zero and fit in constraint_length bits
eng::comms::encode: every generator must be non-zero and fit in constraint_length bits
eng::comms::encode: bits must be zeros and ones
```
### Example 6

Rejected LDPC input.

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
static pluss::Series soft_of(const pluss::Series &codeword, double amplitude) {
  return pluss::series::make(std::vector<double>{1.0}).add(codeword.to(pluss::ScalarType::Float64).mul(-2.0)).mul(amplitude);
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
  report([&] { (void)cm::gallager(1000, 3, 6, 1); });
  report([&] { (void)cm::gallager(1200, 1, 6, 1); });
  report([&] { (void)cm::gallager(1200, 6, 6, 1); });
  const cm::LdpcCode code = cm::gallager(600, 3, 6, 1);
  report([&] { (void)cm::encode(code, random_bits(5, 1)); });
  report([&] { (void)cm::encode(code, vec({0, 2, 1})); });
  report([&] { (void)cm::decode(code, vec({1, 2, 3})); });
  report([&] { (void)cm::decode(code, pluss::series::zeros({600}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), 0); });
  report([&] { (void)cm::decode(cm::LdpcCode{}, vec({1, 2, 3})); });
}
```

Output

```text
eng::comms::gallager: need column_weight >= 2, row_weight > column_weight, and length a multiple of row_weight, at least twice it, and at most 20000
eng::comms::gallager: need column_weight >= 2, row_weight > column_weight, and length a multiple of row_weight, at least twice it, and at most 20000
eng::comms::gallager: need column_weight >= 2, row_weight > column_weight, and length a multiple of row_weight, at least twice it, and at most 20000
eng::comms::encode: bits must be a real [..., k] array with k = code.dimension
eng::comms::encode: bits must be a real [..., k] array with k = code.dimension
eng::comms::decode: soft must be a real floating [..., n] array with n = code.length
eng::comms::decode: every soft value is exactly 0 or 1, which are bits, not log-likelihood ratios; pass the output of llr (positive favours zero) or +1 / -1 for hard decisions
eng::comms::decode: the code is not one made by gallager
```
### Example 7

Rejected turbo input.

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
  const cm::TurboCode code;
  const pluss::Series pi = cm::shuffle(40, 1);
  const pluss::Series bits = random_bits(40, 2);
  report([&] { (void)cm::shuffle(1, 1); });
  report([&] { (void)cm::encode(code, pi.slice(0, 0, 10), bits); });
  report([&] { (void)cm::encode(code, pi.sub(pi), bits); });
  report([&] { (void)cm::encode(cm::TurboCode{4, 013, 05}, pi, bits); });
  report([&] { (void)cm::encode(cm::TurboCode{1, 1, 1}, pi, bits); });
  report([&] { (void)cm::decode(code, pi, pluss::series::zeros({100}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), 2); });
  report([&] { (void)cm::decode(code, pi, pluss::series::zeros({126}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), 0); });
}
```

Output

```text
eng::comms::shuffle: length must be at least 2
eng::comms::encode: interleaver must be an integer vector with one entry per information bit
eng::comms::encode: interleaver must be a permutation of 0 .. length - 1
eng::comms::encode: the feedback polynomial must have its leading bit set
eng::comms::encode: constraint_length must lie in [2, 8]
eng::comms::decode: soft must be a real [..., 3 N + 2 (constraint_length - 1)] array
eng::comms::decode: every soft value is exactly 0 or 1, which are bits, not log-likelihood ratios; pass the output of llr (positive favours zero) or +1 / -1 for hard decisions
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConvolutionalCode.html">
    <span class="label">Previous</span>
    <span class="title">ConvolutionalCode</span>
  </a>
  <a class="next-link" href="decode.html">
    <span class="label">Next</span>
    <span class="title">decode</span>
  </a>
</div>
<!-- apiref:nav:end -->
