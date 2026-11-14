<!-- generated from rrc.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::rrc`

A root-raised-cosine pulse.

## Signature

```cpp
pluss::Series comms::rrc(double rolloff, std::int64_t samples_per_symbol, std::int64_t span, pluss::ScalarType dtype = pluss::ScalarType::Float64);
```

## What It Does

Returns the taps of a root-raised-cosine pulse: `span * samples_per_symbol + 1` samples, symmetric about the centre, scaled to unit energy. Two in cascade (transmit and matched receive filter) make a raised-cosine pulse, which is zero at every other symbol instant: no intersymbol interference.

## Parameters

`rolloff`
: The excess bandwidth, in `[0, 1]`. `0` is the narrowest, a sinc; `1` the widest.

`samples_per_symbol`
: The oversampling factor, at least `2`.

`span`
: The length in symbols, at least `2`.

`dtype`
: `Float64` or `Float32`.

## Returns

A real vector of `span * samples_per_symbol + 1` taps.

## Errors

Throws `pluss::Error` for a `rolloff` outside `[0, 1]`, `samples_per_symbol` below `2`, `span` below `2`, or a `dtype` that is not `Float64` or `Float32`.

## Interpretation and Use

Truncating the pulse leaves a small residual interference that falls with the span. At a span of 16 symbols the interference is below `5e-4` for a rolloff of `0.25` and above, and a rolloff of zero is the slowest to decay. Unit energy makes the noise variance after the matched filter equal to the per-sample variance.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The taps of a short pulse.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const pluss::Series taps = cm::rrc(0.35, 4, 6);
  out.writeln("taps", taps.numel(), " (span 6 symbols x 4 samples + 1)");
  out.writeln("energy", taps.mul(taps).sum().item<double>());
  out.writeln("centre tap", flat(taps)[12], " first and last", flat(taps)[0], flat(taps)[24]);
  out.writeln("symmetric", taps.sub(taps.flip({0})).abs().amax().item<double>() < 1e-14);
}
```

Output

```text
taps 25  (span 6 symbols x 4 samples + 1)
energy 1.00000
centre tap 0.54795  first and last -0.01273 -0.01273
symmetric true
```
### Example 2

Interference against the rolloff.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  // the pulse paired with itself must be zero at every other symbol instant (no intersymbol interference)
  for (double rolloff : {0.0, 0.25, 0.5, 1.0}) {
    const pluss::Series taps = cm::rrc(rolloff, 8, 16);
    const pluss::Series both = pluss::subsp::eng::signal::conv(taps, taps);
    const std::vector<double> r = flat(both);
    const std::size_t mid = r.size() / 2;
    double worst = 0.0;
    for (int k = 1; k <= 8; ++k) worst = std::max(worst, std::abs(r[mid + 8 * static_cast<std::size_t>(k)]));
    out.writeln("rolloff", rolloff, " peak", r[mid], " largest interference at symbol instants", worst);
  }
}
```

Output

```text
rolloff 0.00000  peak 1.00000  largest interference at symbol instants 0.05733
rolloff 0.25000  peak 1.00000  largest interference at symbol instants 0.00047
rolloff 0.50000  peak 1.00000  largest interference at symbol instants 0.00010
rolloff 1.00000  peak 1.00000  largest interference at symbol instants 0.00012
```
### Example 3

Single-precision taps.

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
  out.print_defaults(PrintOptions{}.fixed(7));
  const pluss::Series taps = cm::rrc(0.35, 4, 6, pluss::ScalarType::Float32);
  out.writeln("single precision taps", taps.scalar_type() == pluss::ScalarType::Float32);
  out.writeln("energy", taps.to(pluss::ScalarType::Float64).mul(taps.to(pluss::ScalarType::Float64)).sum().item<double>());
}
```

Output

```text
single precision taps true
energy 1.0000000
```
### Example 4

Rejected parameters.

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
  report([&] { (void)cm::rrc(1.5, 8, 10); });
  report([&] { (void)cm::rrc(-0.1, 8, 10); });
  report([&] { (void)cm::rrc(0.35, 1, 10); });
  report([&] { (void)cm::rrc(0.35, 8, 1); });
  report([&] { (void)cm::rrc(0.35, 8, 10, pluss::ScalarType::Int64); });
}
```

Output

```text
eng::comms::rrc: rolloff must lie in [0, 1]
eng::comms::rrc: rolloff must lie in [0, 1]
eng::comms::rrc: samples_per_symbol must be at least 2
eng::comms::rrc: span must be at least 2 symbols
eng::comms::rrc: dtype must be Float64 or Float32
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
<!-- apiref:nav:end -->
