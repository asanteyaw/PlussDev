<!-- generated from cfo.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::cfo`

The carrier frequency offset of PSK symbols.

## Signature

```cpp
pluss::Series comms::cfo(const pluss::Series& symbols, std::int64_t order);
```

## What It Does

Raises the symbols to the power `order`, which removes the modulation of an M-PSK signal and leaves a tone at `order` times the offset, finds the peak of its zero-padded spectrum and refines it by parabolic interpolation.

## Parameters

`symbols`
: Received symbols, a complex `[..., N]` array with `N >= 16`.

`order`
: The PSK order: `2`, `4` or `8`.

## Returns

A real array of shape `[...]`: the offset in cycles per symbol, in `(-0.5 / order, 0.5 / order)`.

## Errors

Throws `pluss::Error` for an order other than `2`, `4` or `8`, or symbols that are not a complex array of at least sixteen.

## Interpretation and Use

The range is limited to a fraction `1 / (2 order)` of the symbol rate. The accuracy improves with the number of symbols and the signal-to-noise ratio; use the estimate with `derot`, and a `pll` loop for the residual and for drift.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Estimates against noise and order.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  // QPSK symbols with a carrier offset of 0.0031 cycles per symbol and a phase of 0.4 rad, with a little noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series data = cm::modulate(qpsk, random_bits(2 * 3000, 81));
  const pluss::Series zero = pluss::series::make(std::vector<double>{0.0}).select(0, 0);
  const pluss::Series rotated = cm::derot(data, pluss::series::make(std::vector<double>{-0.0031}).select(0, 0), pluss::series::make(std::vector<double>{-0.4}).select(0, 0));
  for (double n0 : {0.0, 0.05, 0.2}) {
    const pluss::Series received = n0 > 0.0 ? cm::noise(rotated, n0, 3) : rotated;
    out.writeln("noise variance", n0, " estimated offset", cm::cfo(received, 4).item<double>(), " true 0.003100");
  }
  // BPSK and 8-PSK use the matching power
  out.writeln("BPSK estimate", cm::cfo(cm::derot(cm::modulate(cm::psk(2), random_bits(3000, 4)), pluss::series::make(std::vector<double>{-0.01}).select(0, 0), zero), 2).item<double>());
}
```

Output

```text
noise variance 0.000000  estimated offset 0.003100  true 0.003100
noise variance 0.050000  estimated offset 0.003100  true 0.003100
noise variance 0.200000  estimated offset 0.003101  true 0.003100
BPSK estimate 0.010000
```
### Example 2

One estimate per frame, and the correction.

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
  // one offset per frame
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(3 * 2 * 500, 5).reshape({3, 1000}));
  const pluss::Series offsets = vec({0.001, 0.002, -0.003});
  const pluss::Series rotated = cm::derot(data, offsets.neg(), vec({0.0, 0.0, 0.0}));
  out.writeln("true offsets     ", flat(offsets));
  out.writeln("estimated offsets", flat(cm::cfo(rotated, 4)));
  out.writeln("restored by derotating with the estimates", cm::derot(rotated, cm::cfo(rotated, 4), vec({0.0, 0.0, 0.0})).sub(data).abs().amax().item<double>() < 0.02);
}
```

Output

```text
true offsets      [0.00100, 0.00200, -0.00300]
estimated offsets [0.00100, 0.00200, -0.00300]
restored by derotating with the estimates true
```
### Example 3

Rejected arguments.

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
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(200, 1));
  report([&] { (void)cm::cfo(data, 3); });
  report([&] { (void)cm::cfo(data.slice(-1, 0, 8), 4); });
  report([&] { (void)cm::cfo(random_bits(200, 1), 4); });
}
```

Output

```text
eng::comms::cfo: order must be 2, 4 or 8
eng::comms::cfo: symbols must be a complex [..., N] array with N >= 16
eng::comms::cfo: symbols must be a complex [..., N] array with N >= 16
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="delay.html">
    <span class="label">Previous</span>
    <span class="title">delay</span>
  </a>
  <a class="next-link" href="derot.html">
    <span class="label">Next</span>
    <span class="title">derot</span>
  </a>
</div>
<!-- apiref:nav:end -->
