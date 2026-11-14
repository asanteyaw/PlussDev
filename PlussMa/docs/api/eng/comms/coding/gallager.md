<!-- generated from gallager.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::gallager`

A regular Gallager LDPC code.

## Signature

```cpp
comms::LdpcCode comms::gallager(std::int64_t length, std::int64_t column_weight, std::int64_t row_weight, std::uint64_t seed);
```

## What It Does

Builds a regular low-density parity-check code by Gallager's construction: `column_weight` stacked bands, the first with `row_weight` consecutive ones in each row and the others random column permutations of it, so that every column has `column_weight` ones and every row `row_weight`. Gauss-Jordan elimination over GF(2) then gives the systematic generator.

## Parameters

`length`
: The codeword length `n`: a multiple of `row_weight`, at least twice it, and at most `20000`.

`column_weight`
: The ones per column, at least `2`.

`row_weight`
: The ones per row, greater than `column_weight`.

`seed`
: The seed of the permutations: the same seed gives the same code.

## Returns

An `LdpcCode`.

## Errors

Throws `pluss::Error` when the weights or the length do not satisfy the conditions above.

## Interpretation and Use

A (3, 6) code is rate one half and decodes within about two decibels of the Shannon limit at long lengths; the construction is cheap but dense in memory (`m x n` entries), so very long codes need a sparse representation. A larger `column_weight` helps less than a longer length.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The code and its checks.

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

Coding gain over uncoded BPSK.

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

Rejected parameters.

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
  <a class="prev-link" href="LdpcCode.html">
    <span class="label">Previous</span>
    <span class="title">LdpcCode</span>
  </a>
  <a class="next-link" href="LdpcResult.html">
    <span class="label">Next</span>
    <span class="title">LdpcResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
