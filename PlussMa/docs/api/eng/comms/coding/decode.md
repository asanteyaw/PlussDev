<!-- generated from decode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::decode`

Decoding a convolutional, LDPC or turbo code from soft values.

## Signature

```cpp
pluss::Series comms::decode(const comms::ConvolutionalCode& code, const pluss::Series& soft);

comms::LdpcResult comms::decode(const comms::LdpcCode& code, const pluss::Series& soft, std::int64_t max_iterations = 50, double scale = 0.75);

pluss::Series comms::decode(const comms::TurboCode& code, const pluss::Series& interleaver, const pluss::Series& soft, std::int64_t iterations = 8);
```

## What It Does

**Convolutional:** runs the Viterbi algorithm over the trellis: at every step it keeps, for each of the `2^(K - 1)` states, the best path (the one that maximises the correlation of its coded bits with the soft values), and traces back from the zero state, where a terminated codeword ends; it is vectorized over the states and every frame. **LDPC:** runs normalised min-sum belief propagation: every check sends each variable the sign product and the smallest magnitude of the other variables' messages, attenuated by `scale`; every variable sums the channel value and the check messages; the loop stops when every frame satisfies every check or after `max_iterations`. **Turbo:** two max-log-MAP (BCJR) decoders exchange extrinsic information through the interleaver for `iterations` rounds, scaling the extrinsic values by `0.75`.

## Parameters

`code`
: The code: a `ConvolutionalCode`, an `LdpcCode` or a `TurboCode`; the overload is chosen by its type.

`interleaver`
: Turbo only: the interleaver the encoder used.

`soft`
: A log-likelihood ratio per coded bit, positive favouring zero, a real floating array `[..., coded length]` in the layout of `encode`; use `+1` and `-1` for hard decisions. Values that are all exactly `0` or `1` (with at least one zero) are bits, not ratios, and are refused.

`max_iterations`
: LDPC only: the largest number of iterations, at least `1`.

`scale`
: LDPC only: the normalisation of the check messages, in `(0, 1]`; `0.75` is the usual choice and `1` is plain min-sum.

`iterations`
: Turbo only: the number of rounds, at least `1`.

## Returns

The convolutional and turbo overloads return an `Int64` array of the information bits, `[..., N]`. The LDPC overload returns an `LdpcResult` with the information bits `[..., k]`, the hard-decision codeword, a convergence flag per frame and the iteration count.

## Errors

Throws `pluss::Error` for an invalid code or interleaver, `soft` that is not real floating of the right layout or that holds bits instead of ratios, or iteration controls out of range.

## Interpretation and Use

Soft values decode better than hard decisions of the same symbols (about two decibels for the convolutional code). The convolutional decoder corrects any pattern of up to four scattered coded-bit errors for the default code and a long burst defeats it; its memory is proportional to the number of coded steps times the number of states. LDPC memory is `m x n` per frame, a frame that does not converge in a hundred iterations usually never will, and a scale below `0.6` slows convergence; `converged` says the output is a codeword, which is almost always the transmitted one. The turbo error rate falls with the iterations and flattens after about eight.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Noiseless Viterbi decoding.

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

static pluss::Series soft_of(const pluss::Series &bits) { return pluss::series::make(std::vector<double>{1.0}).add(bits.to(pluss::ScalarType::Float64).mul(-2.0)); }

int main() {
  OutputEngine out;
  // hard decisions are +1 for a zero and -1 for a one
  const cm::ConvolutionalCode code;
  const pluss::Series bits = random_bits(2 * 200, 9).reshape({2, 200});
  const pluss::Series decoded = cm::decode(code, soft_of(cm::encode(code, bits)));
  out.writeln("decoded", decoded.sizes(), " dtype Int64", decoded.scalar_type() == pluss::ScalarType::Int64);
  out.writeln("every bit recovered", decoded.sub(bits).abs().amax().item<double>() == 0.0);
}
```

Output

```text
decoded [2, 200]  dtype Int64 true
every bit recovered true
```
### Example 2

Isolated errors corrected, a burst not.

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

static pluss::Series soft_of(const pluss::Series &bits) { return pluss::series::make(std::vector<double>{1.0}).add(bits.to(pluss::ScalarType::Float64).mul(-2.0)); }

int main() {
  OutputEngine out;
  // the code has free distance 10: isolated bit errors in the codeword are corrected, a burst is not
  const cm::ConvolutionalCode code;
  const pluss::Series bits = random_bits(300, 12);
  const pluss::Series coded = cm::encode(code, bits).to(pluss::ScalarType::Float64);
  for (int errors : {4, 8, 12}) {
    std::vector<double> mask(static_cast<std::size_t>(coded.numel()), 0.0);
    for (int e = 0; e < errors; ++e) mask[static_cast<std::size_t>(40 + 53 * e)] = 1.0;
    const pluss::Series flipped = coded.sub(pluss::series::make(mask).mul(2.0).mul(coded.sub(0.5)));
    const pluss::Series decoded = cm::decode(code, soft_of(flipped));
    out.writeln(errors, "scattered coded-bit errors: information bit errors", cm::ber(bits, decoded).item<double>() * 300.0);
  }
  std::vector<double> burst(static_cast<std::size_t>(coded.numel()), 0.0);
  for (int e = 100; e < 112; ++e) burst[static_cast<std::size_t>(e)] = 1.0;
  const pluss::Series flipped = coded.sub(pluss::series::make(burst).mul(2.0).mul(coded.sub(0.5)));
  out.writeln("a burst of 12 adjacent coded bits: information bit errors", cm::ber(bits, cm::decode(code, soft_of(flipped))).item<double>() * 300.0);
}
```

Output

```text
4 scattered coded-bit errors: information bit errors 0
8 scattered coded-bit errors: information bit errors 0
12 scattered coded-bit errors: information bit errors 0
a burst of 12 adjacent coded bits: information bit errors 3
```
### Example 3

Convolutional coding gain.

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
  const pluss::Series sent = cm::encode(code, bits);
  out.writeln("Eb/N0   uncoded     coded");
  for (double ebn0 : {2.0, 3.0, 4.0}) {
    const double n0_coded = cm::n0(ebn0, bpsk) / 0.5;
    const pluss::Series received = cm::noise(cm::modulate(bpsk, sent), n0_coded, 7);
    const pluss::Series decoded = cm::decode(code, cm::llr(bpsk, received, n0_coded));
    const pluss::Series raw = cm::noise(cm::modulate(bpsk, bits), cm::n0(ebn0, bpsk), 8);
    out.writeln(ebn0, cm::ber(bits, cm::demod(bpsk, raw)).item<double>(), cm::ber(bits, decoded).item<double>());
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
### Example 4

Soft against hard decisions.

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
  const double n0 = cm::n0(3.0, bpsk) / 0.5;
  const pluss::Series received = cm::noise(cm::modulate(bpsk, cm::encode(code, bits)), n0, 16);
  const pluss::Series soft = cm::decode(code, cm::llr(bpsk, received, n0));
  const pluss::Series hard_bits = cm::demod(bpsk, received);
  const pluss::Series hard = cm::decode(code, pluss::series::make(std::vector<double>{1.0}).add(hard_bits.to(pluss::ScalarType::Float64).mul(-2.0)));
  out.writeln("bit error rate with soft decisions", cm::ber(bits, soft).item<double>(), " with hard decisions", cm::ber(bits, hard).item<double>());
}
```

Output

```text
bit error rate with soft decisions 0.00075  with hard decisions 0.03490
```
### Example 5

LDPC against uncoded BPSK.

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
### Example 6

LDPC iterations and the scale.

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
### Example 7

Turbo against uncoded BPSK.

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
### Example 8

Turbo iterations.

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
### Example 9

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
  const cm::ConvolutionalCode code;
  report([&] { (void)cm::decode(code, pluss::series::zeros({5}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
  report([&] { (void)cm::decode(code, random_bits(40, 1)); });
  report([&] { (void)cm::decode(code, pluss::Series{}); });
  report([&] { (void)cm::decode(cm::ConvolutionalCode{20, {1, 1}}, pluss::series::zeros({40}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
}
```

Output

```text
eng::comms::decode: soft must be a real [..., (N + constraint_length - 1) * generators] array
eng::comms::decode: soft must be a real [..., (N + constraint_length - 1) * generators] array
eng::comms::decode: soft must be a real [..., (N + constraint_length - 1) * generators] array
eng::comms::decode: constraint_length must lie in [2, 12]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="LdpcCode.html">
    <span class="label">Next</span>
    <span class="title">LdpcCode</span>
  </a>
</div>
<!-- apiref:nav:end -->
