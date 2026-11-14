<!-- generated from psk.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::psk`

An M-ary phase-shift-keying constellation.

## Signature

```cpp
comms::Constellation comms::psk(std::int64_t order, const comms::ConstellationOptions& options = {});
```

## What It Does

Builds `order` points on the unit circle, point `k` at the angle `2 pi k / order + phase`. With Gray labelling (the default) the label of point `k` is `k ^ (k >> 1)`, so neighbouring points differ in one bit. Order 2 is BPSK and order 4 is QPSK.

## Parameters

`order`
: The number of points, a power of two of at least `2`.

`options`
: The labelling, rotation and precision; see `ConstellationOptions`.

## Returns

A `Constellation` with `bits = log2(order)` and unit average energy.

## Errors

Throws `pluss::Error` when `order` is not a power of two, `options.phase` is not finite or `options.dtype` is not complex.

## Interpretation and Use

BPSK and QPSK have the same bit error probability at the same Eb/N0; higher orders pack more bits per symbol but need more energy per bit for the same error rate.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

QPSK points by label.

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
  // QPSK: four points on the unit circle; the default labelling is Gray, so label 2 is at angle 3 pi / 2 and label 3 at angle pi
  const cm::Constellation qpsk = cm::psk(4);
  out.writeln("order", qpsk.order, " bits per symbol", qpsk.bits);
  out.writeln("real parts by label", flat(qpsk.points.real()));
  out.writeln("imaginary parts by label", flat(qpsk.points.imag()));
  const cm::Constellation eight = cm::psk(8);
  out.writeln("8-PSK point of label 5 (Gray)", flat(eight.points.real())[5], flat(eight.points.imag())[5]);
}
```

Output

```text
order 4  bits per symbol 2
real parts by label [1.0000, 0.0000, -0.0000, -1.0000]
imaginary parts by label [0.0000, 1.0000, -1.0000, 0.0000]
8-PSK point of label 5 (Gray) -0.0000 -1.0000
```
### Example 2

Gray against natural labels.

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
  const auto neighbours = [&](const cm::Constellation &c, const char *name) {
    // walk once round the circle and count the bit changes between neighbouring points
    const std::vector<double> re = flat(c.points.real()), im = flat(c.points.imag());
    std::vector<std::int64_t> by_angle(re.size());
    for (std::size_t label = 0; label < re.size(); ++label) {
      double angle = std::atan2(im[label], re[label]);
      if (angle < -1e-9) angle += 2.0 * M_PI;
      by_angle[static_cast<std::size_t>(std::llround(angle / (2.0 * M_PI / static_cast<double>(re.size())))) % re.size()] = static_cast<std::int64_t>(label);
    }
    std::vector<int> changes;
    for (std::size_t k = 0; k < by_angle.size(); ++k) {
      std::int64_t x = by_angle[k] ^ by_angle[(k + 1) % by_angle.size()];
      int n = 0;
      for (; x; x >>= 1) n += static_cast<int>(x & 1);
      changes.push_back(n);
    }
    out.writeln(name, "labels round the circle", by_angle, " bit changes between neighbours", changes);
  };
  neighbours(cm::psk(8), "Gray   ");
  cm::ConstellationOptions natural;
  natural.gray = false;
  neighbours(cm::psk(8, natural), "natural");
}
```

Output

```text
Gray    labels round the circle [0, 1, 3, 2, 6, 7, 5, 4]  bit changes between neighbours [1, 1, 1, 1, 1, 1, 1, 1]
natural labels round the circle [0, 1, 2, 3, 4, 5, 6, 7]  bit changes between neighbours [1, 2, 1, 3, 1, 2, 1, 3]
```
### Example 3

Rotation and single precision.

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
  // a phase offset rotates every point; Complex64 gives single-precision points
  cm::ConstellationOptions options;
  options.phase = M_PI / 4;
  const cm::Constellation rotated = cm::psk(4, options);
  out.writeln("QPSK at 45 degrees, point of label 0", flat(rotated.points.real())[0], flat(rotated.points.imag())[0]);
  cm::ConstellationOptions single;
  single.dtype = pluss::ScalarType::Complex64;
  out.writeln("single precision points", cm::psk(4, single).points.scalar_type() == pluss::ScalarType::Complex64);
  // pi/2-BPSK style start: BPSK is two points on the real axis
  out.writeln("BPSK real parts", flat(cm::psk(2).points.real()));
}
```

Output

```text
QPSK at 45 degrees, point of label 0 0.70711 0.70711
single precision points true
BPSK real parts [1.00000, -1.00000]
```
### Example 4

Rejected orders.

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
  report([&] { (void)cm::psk(6); });
  report([&] { (void)cm::psk(1); });
  cm::ConstellationOptions bad;
  bad.phase = std::nan("");
  report([&] { (void)cm::psk(4, bad); });
  cm::ConstellationOptions real;
  real.dtype = pluss::ScalarType::Float64;
  report([&] { (void)cm::psk(4, real); });
}
```

Output

```text
eng::comms::psk: order must be a power of two (2, 4, 8, ...)
eng::comms::psk: order must be a power of two (2, 4, 8, ...)
eng::comms::psk: phase must be finite
eng::comms: constellation dtype must be Complex128 or Complex64
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConstellationOptions.html">
    <span class="label">Previous</span>
    <span class="title">ConstellationOptions</span>
  </a>
  <a class="next-link" href="qam.html">
    <span class="label">Next</span>
    <span class="title">qam</span>
  </a>
</div>
<!-- apiref:nav:end -->
