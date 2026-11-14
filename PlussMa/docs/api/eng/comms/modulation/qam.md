<!-- generated from qam.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::qam`

A square M-ary quadrature-amplitude-modulation constellation.

## Signature

```cpp
comms::Constellation comms::qam(std::int64_t order, const comms::ConstellationOptions& options = {});
```

## What It Does

Builds a square grid of `order` points: the high half of a label chooses the in-phase level and the low half the quadrature level, each from `sqrt(order)` equally spaced amplitudes, Gray-coded by default. The points are scaled to unit average energy.

## Parameters

`order`
: The number of points, a power of four of at least `4`: `4`, `16`, `64`, `256`.

`options`
: The labelling, rotation and precision; see `ConstellationOptions`.

## Returns

A `Constellation` with `bits = log2(order)` and unit average energy.

## Errors

Throws `pluss::Error` when `order` is not a power of four (non-square QAM such as 8 and 32 is not supported), `options.phase` is not finite or `options.dtype` is not complex.

## Interpretation and Use

With Gray labels the nearest neighbours of every point differ in one bit. The distance between neighbouring points shrinks as the order grows, so a larger order needs a higher signal-to-noise ratio.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The 16-QAM grid.

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
  out.writeln("order", c.order, " bits per symbol", c.bits);
  const std::vector<double> re = flat(c.points.real()), im = flat(c.points.imag());
  out.writeln("distinct in-phase levels", std::vector<double>{re[0], re[4], re[8], re[12]});
  double energy = 0.0;
  for (std::size_t k = 0; k < re.size(); ++k) energy += re[k] * re[k] + im[k] * im[k];
  out.writeln("average energy", energy / 16.0);
  out.writeln("label 0b0000 and label 0b1111", re[0], im[0], re[15], im[15]);
}
```

Output

```text
order 16  bits per symbol 4
distinct in-phase levels [-0.9487, -0.3162, 0.9487, 0.3162]
average energy 1.0000
label 0b0000 and label 0b1111 -0.9487 -0.9487 0.3162 0.3162
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
  // the closest pairs of points differ in exactly one bit when the labels are Gray coded, and in more when natural
  for (bool gray : {true, false}) {
    cm::ConstellationOptions options;
    options.gray = gray;
    const cm::Constellation c = cm::qam(16, options);
    const std::vector<double> re = flat(c.points.real()), im = flat(c.points.imag());
    double nearest = 1e9;
    for (std::size_t a = 0; a < 16; ++a)
      for (std::size_t b = a + 1; b < 16; ++b) nearest = std::min(nearest, std::hypot(re[a] - re[b], im[a] - im[b]));
    int worst = 0;
    for (std::size_t a = 0; a < 16; ++a)
      for (std::size_t b = a + 1; b < 16; ++b)
        if (std::abs(std::hypot(re[a] - re[b], im[a] - im[b]) - nearest) < 1e-9) {
          int n = 0;
          for (std::size_t x = a ^ b; x; x >>= 1) n += static_cast<int>(x & 1);
          worst = std::max(worst, n);
        }
    out.writeln(gray ? "Gray   " : "natural", " most bits that differ between nearest neighbours", worst);
  }
}
```

Output

```text
Gray     most bits that differ between nearest neighbours 1
natural  most bits that differ between nearest neighbours 2
```
### Example 3

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
  report([&] { (void)cm::qam(8); });
  report([&] { (void)cm::qam(2); });
  report([&] { (void)cm::qam(32); });
  report([&] { (void)cm::qam(0); });
}
```

Output

```text
eng::comms::qam: order must be a power of four (4, 16, 64, ...): square QAM only
eng::comms::qam: order must be a power of four (4, 16, 64, ...): square QAM only
eng::comms::qam: order must be a power of four (4, 16, 64, ...): square QAM only
eng::comms::qam: order must be a power of four (4, 16, 64, ...): square QAM only
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="psk.html">
    <span class="label">Previous</span>
    <span class="title">psk</span>
  </a>
  <a class="next-link" href="modulate.html">
    <span class="label">Next</span>
    <span class="title">modulate</span>
  </a>
</div>
<!-- apiref:nav:end -->
