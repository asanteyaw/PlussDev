<!-- generated from iterative_codes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# LDPC and turbo codes

[Back to Communications 1: Modulation and Detection](../comms_01_modulation.md)

Convolutional codes with a Viterbi decoder get a few decibels. Codes that are decoded **iteratively** get within one or two decibels of the Shannon limit at long block lengths. This page builds a low-density parity-check (LDPC) code and a turbo code, decodes them from log-likelihood ratios, and measures the gain over uncoded BPSK and the effect of the iterations.

## The mental model

**LDPC.** A code is the set of bit vectors that satisfy a sparse parity-check matrix `H` (every row a parity equation of a few bits). `gallager(length, column_weight, row_weight, seed)` builds a regular Gallager code and derives a systematic generator by elimination over GF(2); `encode` multiplies the information bits by it. `decode` runs **belief propagation** (normalised min-sum): every parity check tells each of its bits what the other bits imply, every bit adds up what it is told, and the loop stops as soon as every check holds. It returns a Bool per frame: `converged` means the output is a codeword.

**Turbo.** Two identical recursive systematic convolutional encoders, the second reading the bits through a random **interleaver**, send the bits and two parity streams (rate about one third). `decode` runs two max-log-MAP (BCJR) decoders that pass **extrinsic** information to each other through the interleaver, for a number of iterations. The interleaver must be the same at both ends.

Both take the log-likelihood ratios of `llr` (positive favouring zero) and give bits.

## An LDPC code

A `(3, 6)` regular code of length `1200`: every column has `3` ones and every row `6`, so the rate is one half (`602 / 1200`: two checks are dependent).

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
  // a regular (3, 6) Gallager code of length 1200: every column has 3 ones and every row 6
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  out.writeln("length", code.length, " dimension", code.dimension, " rate", static_cast<double>(code.dimension) / static_cast<double>(code.length));
  out.writeln("parity checks", code.parity_check.sizes(), " generator", code.generator.sizes());
  out.writeln("column weights (first five)", flat(code.parity_check.to(pluss::ScalarType::Float64).sum(0).slice(0, 0, 5)), " row weights (first five)", flat(code.parity_check.to(pluss::ScalarType::Float64).sum(1).slice(0, 0, 5)));
  const pluss::Series syndrome = code.generator.to(pluss::ScalarType::Float64).matmul(code.parity_check.to(pluss::ScalarType::Float64).mT());
  out.writeln("every generator row satisfies every check", syndrome.sub(syndrome.div(2.0).floor().mul(2.0)).abs().amax().item<double>() == 0.0);
  out.writeln("same seed, same code", cm::gallager(1200, 3, 6, 1).parity_check.sub(code.parity_check).abs().amax().item<std::int64_t>() == 0,
              " different seed, different code", cm::gallager(1200, 3, 6, 2).parity_check.sub(code.parity_check).abs().amax().item<std::int64_t>() != 0);
}
```

Output

```text
length 1200  dimension 602  rate 0.5016666666666667
parity checks [600, 1200]  generator [602, 1200]
column weights (first five) [3, 3, 3, 3, 3]  row weights (first five) [6, 6, 6, 6, 6]
every generator row satisfies every check true
same seed, same code true  different seed, different code true
```

## Encoding and decoding

The encoder is systematic: the information bits sit at the info positions of the codeword. With no noise the decoder finds the codeword after one iteration.

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

## Coding gain

The program sends six frames over BPSK at four values of `Eb/N0`, with the rate-1/2 code against uncoded BPSK at the same energy per information bit.

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
  // BPSK with the rate-1/2 (3, 6) code against uncoded BPSK at the same Eb/N0
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  const double rate = static_cast<double>(code.dimension) / 1200.0;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(code.dimension * 6, 32).reshape({6, code.dimension});
  const pluss::Series sent = cm::modulate(bpsk, cm::encode(code, bits));
  out.writeln("Eb/N0   uncoded     coded     frames decoded");
  for (double ebn0 : {1.5, 2.0, 2.5, 3.0}) {
    const double n0 = cm::n0(ebn0, bpsk) / rate;
    const cm::LdpcResult decoded = cm::decode(code, cm::llr(bpsk, cm::noise(sent, n0, 33), n0), 60);
    const pluss::Series raw = cm::noise(cm::modulate(bpsk, bits), cm::n0(ebn0, bpsk), 34);
    out.writeln(ebn0, cm::ber(bits, cm::demod(bpsk, raw)).mean().item<double>(), cm::ber(bits, decoded.bits).mean().item<double>(),
                decoded.converged.to(pluss::ScalarType::Float64).sum().item<double>(), "of 6");
  }
}
```

Output

```text
Eb/N0   uncoded     coded     frames decoded
1.50000 0.04513 0.00831 5.00000 of 6
2.00000 0.03461 0.00000 6.00000 of 6
2.50000 0.02824 0.00000 6.00000 of 6
3.00000 0.02187 0.00000 6.00000 of 6
```

At `2 dB` the uncoded error rate is `3.5 %` and every one of the `3612` coded bits decodes: the code needs about two decibels less than uncoded BPSK needs for a rate of `3 %`, and at `1.5 dB`, near its threshold, one frame of six fails (`0.8 %` bit errors).

## Iterations and the scale

Belief propagation needs a handful of iterations. The program decodes six frames at `2.5 dB` allowing `1`, `3`, `10`, `30` and `100` iterations, and then three scales of the check messages.

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
  // more iterations decode more frames; the decoder stops as soon as every check holds
  const cm::LdpcCode code = cm::gallager(1200, 3, 6, 1);
  const double rate = static_cast<double>(code.dimension) / 1200.0;
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(code.dimension * 6, 35).reshape({6, code.dimension});
  const double n0 = cm::n0(2.5, bpsk) / rate;
  const pluss::Series soft = cm::llr(bpsk, cm::noise(cm::modulate(bpsk, cm::encode(code, bits)), n0, 36), n0);
  out.writeln("iterations allowed  used  bit error rate  frames decoded");
  for (std::int64_t allowed : {1, 3, 10, 30, 100}) {
    const cm::LdpcResult r = cm::decode(code, soft, allowed);
    out.writeln(allowed, r.iterations.items<std::int64_t>()[0], cm::ber(bits, r.bits).mean().item<double>(), r.converged.to(pluss::ScalarType::Float64).sum().item<double>());
  }
  // the scale of the check messages: 0.75 is the usual attenuation of min-sum
  for (double scale : {0.5, 0.75, 1.0}) out.writeln("scale", scale, " bit error rate", cm::ber(bits, cm::decode(code, soft, 50, scale).bits).mean().item<double>());
}
```

Output

```text
iterations allowed  used  bit error rate  frames decoded
1 1 0.04679 0.00000
3 3 0.01800 0.00000
10 9 0.00000 6.00000
30 9 0.00000 6.00000
100 9 0.00000 6.00000
scale 0.50000  bit error rate 0.00360
scale 0.75000  bit error rate 0.00000
scale 1.00000  bit error rate 0.00000
```

One iteration leaves `4.7 %` errors, three `1.8 %`, and ten decode all six frames; the decoder stops as soon as the checks hold (`9` iterations), so allowing more costs nothing. A scale of `0.5` is too cautious (`0.36 %` errors); `0.75` and `1.0` decode every frame here.

## A turbo code

The codeword has the layout systematic, first parity, second parity, then the tail of the first encoder. Twelve bits through the interleaver `1, 4, 8, 9, ...`:

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

`3 x 12 + 2 x 3 = 42` coded bits. The tail returns the first encoder to its zero state, so the decoder knows where it ends; the second encoder is left unterminated.

## Turbo coding gain

The program sends ten blocks of `400` bits at four noise levels and decodes with eight iterations.

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
  // BPSK with the rate-1/3 turbo code against uncoded BPSK at the same Eb/N0
  const cm::TurboCode code;
  const std::int64_t n = 400;
  const pluss::Series pi = cm::shuffle(n, 5);
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(n * 10, 62).reshape({10, n});
  const pluss::Series sent = cm::modulate(bpsk, cm::encode(code, pi, bits));
  const double rate = static_cast<double>(n) / static_cast<double>(3 * n + 6);
  out.writeln("Eb/N0   uncoded     coded (8 iterations)");
  for (double ebn0 : {0.5, 1.0, 1.5, 2.0}) {
    const double n0 = cm::n0(ebn0, bpsk) / rate;
    const pluss::Series decoded = cm::decode(code, pi, cm::llr(bpsk, cm::noise(sent, n0, 63), n0), 8);
    const pluss::Series raw = cm::noise(cm::modulate(bpsk, bits), cm::n0(ebn0, bpsk), 64);
    out.writeln(ebn0, cm::ber(bits, cm::demod(bpsk, raw)).mean().item<double>(), cm::ber(bits, decoded).mean().item<double>());
  }
}
```

Output

```text
Eb/N0   uncoded     coded (8 iterations)
0.50000 0.06425 0.03000
1.00000 0.05250 0.00000
1.50000 0.04275 0.00000
2.00000 0.03400 0.00000
```

At `1 dB` the uncoded error rate is `5.3 %` and the turbo code has none in `4000` bits, and even at `0.5 dB` (half the energy per bit uncoded BPSK needs for the same `6 %`) the coded rate is `3.0 %`.

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
  // each iteration passes extrinsic information between the two decoders: the error rate falls with the iterations
  const cm::TurboCode code;
  const std::int64_t n = 400;
  const pluss::Series pi = cm::shuffle(n, 5);
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series bits = random_bits(n * 10, 62).reshape({10, n});
  const double rate = static_cast<double>(n) / static_cast<double>(3 * n + 6);
  const double n0 = cm::n0(1.5, bpsk) / rate;
  const pluss::Series soft = cm::llr(bpsk, cm::noise(cm::modulate(bpsk, cm::encode(code, pi, bits)), n0, 63), n0);
  out.writeln("iterations  bit error rate");
  for (std::int64_t it : {1, 2, 4, 8, 12}) out.writeln(it, cm::ber(bits, cm::decode(code, pi, soft, it)).mean().item<double>());
  // a different code: constraint length 3 with feedforward 5 and feedback 7
  cm::TurboCode small;
  small.constraint_length = 3;
  small.feedforward = 05;
  small.feedback = 07;
  const pluss::Series small_word = cm::encode(small, pi, bits);
  out.writeln("constraint length 3 codeword length", small_word.sizes(), " = 3 x 400 + 2 x 2");
}
```

Output

```text
iterations  bit error rate
1 0.02800
2 0.00175
4 0.00000
8 0.00000
12 0.00000
constraint length 3 codeword length [10, 1204]  = 3 x 400 + 2 x 2
```

Each iteration passes information between the decoders: one iteration leaves `2.8 %` errors at `1.5 dB`, two `0.18 %`, and four none. A smaller code works the same way (constraint length `3`, feedforward `5`, feedback `7`).

## Errors

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

## Decisions

- **Long blocks, near-capacity, moderate complexity:** LDPC; the decoder stops early on convergence and tells you which frames failed.
- **Short blocks and a fixed rate near one third:** turbo, eight iterations.
- **Both:** feed the decoder `llr` computed with the correct noise variance; a wrong variance scales all the messages.
- **Memory:** LDPC uses `m x n` per frame (dense); keep `n` at a few thousand.

## Pitfalls

The program makes three mistakes: decoding with a different interleaver, giving the decoder bits as soft values, and getting the sign convention backwards.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const cm::Constellation bpsk = cm::psk(2);
  const std::int64_t n = 400;
  const cm::TurboCode turbo;
  const pluss::Series pi = cm::shuffle(n, 5);
  const pluss::Series bits = random_bits(n * 10, 62).reshape({10, n});
  const double rate = static_cast<double>(n) / static_cast<double>(3 * n + 6);
  const double n0 = cm::n0(1.5, bpsk) / rate;
  const pluss::Series soft = cm::llr(bpsk, cm::noise(cm::modulate(bpsk, cm::encode(turbo, pi, bits)), n0, 63), n0);

  // 1. a different interleaver at the receiver: the second decoder's information is scrambled
  out.writeln("bit error rate with the right interleaver  ", cm::ber(bits, cm::decode(turbo, pi, soft, 8)).mean().item<double>());
  out.writeln("bit error rate with another interleaver     ", cm::ber(bits, cm::decode(turbo, cm::shuffle(n, 6), soft, 8)).mean().item<double>());

  // 2. bits as soft values: zeros and ones are not log-likelihood ratios, so the decoder refuses them
  const cm::LdpcCode ldpc = cm::gallager(1200, 3, 6, 1);
  const pluss::Series info = random_bits(ldpc.dimension, 3).reshape({1, ldpc.dimension});
  const pluss::Series word = cm::encode(ldpc, info);
  try {
    (void)cm::decode(ldpc, word.to(pluss::ScalarType::Float64));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }

  // 3. the sign convention: a positive ratio favours zero, so a negated ratio decodes the complement
  const pluss::Series clean = pluss::series::make(std::vector<double>{1.0}).add(word.to(pluss::ScalarType::Float64).mul(-2.0)).mul(4.0);
  out.writeln("bit error rate with the right sign", cm::ber(info, cm::decode(ldpc, clean).bits).mean().item<double>());
  out.writeln("bit error rate with the sign flipped", cm::ber(info, cm::decode(ldpc, clean.neg()).bits).mean().item<double>());
}
```

Output

```text
bit error rate with the right interleaver   0.00000
bit error rate with another interleaver      0.18050
eng::comms::decode: every soft value is exactly 0 or 1, which are bits, not log-likelihood ratios; pass the output of llr (positive favours zero) or +1 / -1 for hard decisions
bit error rate with the right sign 0.00000
bit error rate with the sign flipped 1.00000
```

A turbo decoder given a different interleaver than the encoder used leaves `18 %` of the bits wrong at `1.5 dB`, where the right one leaves none: the second decoder is fed scrambled information, and nothing can tell. Bits used as soft values are refused with a message that names the rule. And the sign convention matters completely: with the sign flipped every bit is wrong (`1.00000`).

## Where next

Reference pages: [`gallager`](../../../../api/eng/comms/coding/gallager.md), [`decode`](../../../../api/eng/comms/coding/decode.md), [`LdpcResult`](../../../../api/eng/comms/coding/LdpcResult.md), [`encode`](../../../../api/eng/comms/coding/encode.md), [`decode`](../../../../api/eng/comms/coding/decode.md), [`TurboCode`](../../../../api/eng/comms/coding/TurboCode.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iterative_codes.html">
    <span class="label">Previous</span>
    <span class="title">LDPC and turbo codes</span>
  </a>
  <a class="next-link" href="synchronisation.html">
    <span class="label">Next</span>
    <span class="title">Synchronisation</span>
  </a>
</div>
<!-- apiref:nav:end -->
