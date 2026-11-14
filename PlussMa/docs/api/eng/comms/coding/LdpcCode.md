<!-- generated from LdpcCode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::LdpcCode`

A binary low-density parity-check code.

## Signature

```cpp
comms::LdpcCode
```

## What It Does

Holds a sparse parity-check matrix, a systematic generator derived from it by Gauss-Jordan elimination over GF(2), and the columns that carry the information bits. `gallager` makes one; the encoder and decoder read it.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `parity_check` | The parity-check matrix `H`, `[m, n]`, zeros and ones (Int64). |  |
| `generator` | The systematic generator `G`, `[k, n]`, with `G H^T = 0` modulo two. |  |
| `info_positions` | The columns of a codeword that hold the information bits, length `k`. |  |
| `length` | The codeword length `n`. |  |
| `dimension` | The number of information bits `k`; at least `n - m`, equal to it when the checks are independent. |  |

## Returns

Returned by `gallager`.

## Errors

`encode` and `decode` throw `pluss::Error` for a code that is not one made by `gallager`.

## Interpretation and Use

The rate is `k / n`, at least `1 - column_weight / row_weight` for a regular code. A code is a value: build it once and reuse it for every frame.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A (3, 6) code of length 1200.

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
### Example 2

Encoding and decoding with it.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="decode.html">
    <span class="label">Previous</span>
    <span class="title">decode</span>
  </a>
  <a class="next-link" href="gallager.html">
    <span class="label">Next</span>
    <span class="title">gallager</span>
  </a>
</div>
<!-- apiref:nav:end -->
