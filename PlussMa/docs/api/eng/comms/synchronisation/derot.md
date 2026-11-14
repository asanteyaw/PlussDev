<!-- generated from derot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::derot`

Removing a carrier offset.

## Signature

```cpp
pluss::Series comms::derot(const pluss::Series& symbols, const pluss::Series& cycles_per_symbol, const pluss::Series& phase);
```

## What It Does

Multiplies symbol `n` by `exp(-j (2 pi f n + phi))`, undoing a carrier frequency offset `f` (in cycles per symbol) and a phase `phi` (in radians).

## Parameters

`symbols`
: Complex symbols, `[..., N]`.

`cycles_per_symbol`
: The frequency offset, a scalar or `[...]`.

`phase`
: The phase offset, a scalar or `[...]`.

## Returns

A complex array of the shape of `symbols`.

## Errors

Throws `pluss::Error` for symbols that are not a complex array.

## Interpretation and Use

Use a negative `f` to apply an offset, for example to build a test signal. The result of `cfo` can be passed directly.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The angles after derotating.

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
  // derotation multiplies symbol n by exp(-j (2 pi f n + phi))
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series symbols = cm::symbols(qpsk, pluss::series::make(std::vector<std::int64_t>{0, 0, 0, 0}));
  const pluss::Series f = pluss::series::make(std::vector<double>{0.05}).select(0, 0), phi = pluss::series::make(std::vector<double>{0.3}).select(0, 0);
  const pluss::Series turned = cm::derot(symbols, f, phi);
  out.writeln("angles of four identical symbols after derotating", flat(turned.imag().atan2(turned.real())));
  out.writeln("expected", -0.3, -0.3 - 2.0 * M_PI * 0.05, -0.3 - 4.0 * M_PI * 0.05, -0.3 - 6.0 * M_PI * 0.05);
  // derotating by the opposite sign restores them: derotate by (-f, -phi) rotates forward
  const pluss::Series back = cm::derot(turned, f.neg(), phi.neg());
  out.writeln("round trip difference", back.sub(symbols).abs().amax().item<double>());
}
```

Output

```text
angles of four identical symbols after derotating [-0.30000, -0.61416, -0.92832, -1.24248]
expected -0.30000 -0.61416 -0.92832 -1.24248
round trip difference 0.00000
```
### Example 2

One offset per frame.

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

Rejected input.

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
  const pluss::Series data = cm::modulate(cm::psk(4), random_bits(40, 1));
  const pluss::Series zero = pluss::series::make(std::vector<double>{0.0}).select(0, 0);
  report([&] { (void)cm::derot(random_bits(40, 1), zero, zero); });
  report([&] { (void)cm::derot(pluss::Series{}, zero, zero); });
}
```

Output

```text
eng::comms::derot: symbols must be a complex [..., N] array
eng::comms::derot: symbols must be a complex [..., N] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cfo.html">
    <span class="label">Previous</span>
    <span class="title">cfo</span>
  </a>
  <a class="next-link" href="CarrierResult.html">
    <span class="label">Next</span>
    <span class="title">CarrierResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
