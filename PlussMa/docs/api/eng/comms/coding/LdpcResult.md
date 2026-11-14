<!-- generated from LdpcResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::LdpcResult`

The result of LDPC decoding.

## Signature

```cpp
comms::LdpcResult
```

## What It Does

Holds the decoded information, the hard-decision codeword and whether the decoder found a codeword.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `bits` | The decoded information bits, `[..., k]`, Int64. |  |
| `codeword` | The hard-decision codeword, `[..., n]`, Int64. |  |
| `converged` | Whether every parity check holds for each frame, a Bool array `[...]`. |  |
| `iterations` | The number of iterations used by the batch, an `Int64` vector of one element. |  |

## Returns

Returned by `decode`.

## Errors

Reading a field does not decode again.

## Interpretation and Use

`converged` true means the output is a codeword, which is almost always the transmitted one; false means the decoder gave up and the bits are the best guess. A decoder that fails to converge is a detected error, unlike a wrong codeword.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field.

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
### Example 2

Frames decoded against Eb/N0.

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
### Example 3

Iterations and convergence.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gallager.html">
    <span class="label">Previous</span>
    <span class="title">gallager</span>
  </a>
  <a class="next-link" href="TurboCode.html">
    <span class="label">Next</span>
    <span class="title">TurboCode</span>
  </a>
</div>
<!-- apiref:nav:end -->
