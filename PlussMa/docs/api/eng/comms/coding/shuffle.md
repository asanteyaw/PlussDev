<!-- generated from shuffle.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::shuffle`

A pseudo-random interleaver.

## Signature

```cpp
pluss::Series comms::shuffle(std::int64_t length, std::uint64_t seed);
```

## What It Does

Returns a permutation of `0 .. length - 1` (the order of a random sort), reproducible from `seed`. The second constituent encoder reads the information bits in this order.

## Parameters

`length`
: The block length `N`, at least `2`.

`seed`
: The seed.

## Returns

An `Int64` vector of length `N`.

## Errors

Throws `pluss::Error` for a length below `2`.

## Interpretation and Use

Transmitter and receiver must use the same interleaver. A random interleaver is a good default; longer blocks give a better code.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

An interleaver and a codeword.

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
### Example 2

The effect of iterations.

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
### Example 3

Rejected lengths.

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
  <a class="prev-link" href="TurboCode.html">
    <span class="label">Previous</span>
    <span class="title">TurboCode</span>
  </a>
</div>
<!-- apiref:nav:end -->
