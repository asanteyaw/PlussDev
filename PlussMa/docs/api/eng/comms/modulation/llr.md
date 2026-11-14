<!-- generated from llr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::llr`

Soft-decision log-likelihood ratios.

## Signature

```cpp
pluss::Series comms::llr(const comms::Constellation& constellation, const pluss::Series& received, double noise_variance);
```

## What It Does

Computes the max-log approximation of `log P(bit = 0) / P(bit = 1)` for every bit: the difference between the squared distance to the nearest point whose bit is one and to the nearest point whose bit is zero, divided by `noise_variance`. A positive value favours the bit being zero, and the magnitude is the confidence.

## Parameters

`constellation`
: The constellation the symbols were sent from.

`received`
: The received complex symbols, `[..., N]`.

`noise_variance`
: The total complex noise variance per symbol, `E|n|^2`, positive.

## Returns

A real `[..., N * bits]` array.

## Errors

Throws `pluss::Error` for a constellation not made by `psk` or `qam`, a non-positive `noise_variance`, or an undefined or scalar `received`.

## Interpretation and Use

The sign of the ratio is the hard decision, and the magnitude says how far the symbol is from the decision boundary: bits with a large magnitude are almost never wrong. For BPSK the result is `4 r / N0`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

BPSK ratios against 4 r / N0.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  // BPSK: label 0 is +1 and label 1 is -1 for the Gray-labelled constellation; the log-likelihood ratio is 4 r / N0
  const cm::Constellation bpsk = cm::psk(2);
  const std::vector<double> points = flat(bpsk.points.real());
  out.writeln("point of label 0 and label 1", points[0], points[1]);
  const pluss::Series received = pluss::series::complex(vec({0.9, 0.1, -0.1, -0.9}), vec({0.0, 0.0, 0.0, 0.0}), pluss::ScalarType::Complex128);
  out.writeln("llr at N0 = 0.5", flat(cm::llr(bpsk, received, 0.5)));
  out.writeln("4 r / N0       ", 4 * 0.9 / 0.5, 4 * 0.1 / 0.5, -4 * 0.1 / 0.5, -4 * 0.9 / 0.5);
  out.writeln("positive favours bit 0, and a larger N0 shrinks the confidence", flat(cm::llr(bpsk, received, 2.0)));
}
```

Output

```text
point of label 0 and label 1 1.000 -1.000
llr at N0 = 0.5 [7.200, 0.800, -0.800, -7.200]
4 r / N0        7.200 0.800 -0.800 -7.200
positive favours bit 0, and a larger N0 shrinks the confidence [1.800, 0.200, -0.200, -1.800]
```
### Example 2

Signs against hard decisions, and confidence against errors.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(4 * 5000, 12);
  const double n0 = cm::n0(8.0, c);
  const pluss::Series received = cm::noise(cm::modulate(c, bits), n0, 17);
  const pluss::Series soft = cm::llr(c, received, n0);
  const pluss::Series hard = cm::demod(c, received);
  out.writeln("llr shape", soft.sizes());
  out.writeln("sign of the llr agrees with the hard decision for all but", cm::ber(hard, soft.lt(0.0).to(pluss::ScalarType::Int64)).item<double>(), "of the bits");
  // the least confident bits are the ones most likely to be wrong
  const pluss::Series wrong = hard.ne(bits).to(pluss::ScalarType::Float64);
  const pluss::Series confident = soft.abs().gt(8.0).to(pluss::ScalarType::Float64);
  out.writeln("error rate overall", wrong.mean().item<double>(), " among bits with |llr| above 8", wrong.mul(confident).sum().item<double>() / confident.sum().item<double>());
}
```

Output

```text
llr shape [20000]
sign of the llr agrees with the hard decision for all but 0.0000 of the bits
error rate overall 0.0089  among bits with |llr| above 8 0.0001
```
### Example 3

Rejected variances.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const cm::Constellation c = cm::psk(4);
  const pluss::Series received = cm::symbols(c, pluss::series::make(std::vector<std::int64_t>{0, 1}));
  report([&] { (void)cm::llr(c, received, 0.0); });
  report([&] { (void)cm::llr(c, received, -1.0); });
  report([&] { (void)cm::llr(c, pluss::Series{}, 0.5); });
}
```

Output

```text
eng::comms::llr: noise_variance must be positive
eng::comms::llr: noise_variance must be positive
eng::comms::llr: received must be a [..., N] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="demod.html">
    <span class="label">Previous</span>
    <span class="title">demod</span>
  </a>
  <a class="next-link" href="to_tones.html">
    <span class="label">Next</span>
    <span class="title">to_tones</span>
  </a>
</div>
<!-- apiref:nav:end -->
