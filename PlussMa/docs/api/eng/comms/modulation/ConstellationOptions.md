<!-- generated from ConstellationOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::ConstellationOptions`

Choices of the constellation builders.

## Signature

```cpp
comms::ConstellationOptions
```

## What It Does

Selects the labelling, the rotation and the precision of the points made by `psk` and `qam`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `gray` | Gray labelling: points that are neighbours differ in exactly one bit, so the usual symbol error flips one bit. `false` labels the points in natural order. | `true` |
| `phase` | A rotation of every point in radians. | `0` |
| `dtype` | The element type of the points, `Complex128` or `Complex64`. | `Complex128` |

## Returns

A value; read by `psk` and `qam`.

## Errors

`psk` and `qam` throw `pluss::Error` for a non-finite `phase` or a `dtype` that is not complex.

## Interpretation and Use

With Gray labels a decision for a neighbouring point costs one bit error; with natural labels it can cost several (on 8-PSK up to three). Gray is the default. A phase offset models a rotated constellation such as pi/4-QPSK. Symbols made from a `Complex64` constellation, and noise added to them, stay in single precision.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Gray against natural labels on 8-PSK.

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
### Example 2

A phase offset and single-precision points.

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
### Example 3

Gray against natural labels on 16-QAM.

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
### Example 4

Single precision through a transmission.

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
  // a Complex64 constellation keeps modulated symbols and noisy symbols in single precision
  cm::ConstellationOptions single;
  single.dtype = pluss::ScalarType::Complex64;
  const cm::Constellation c = cm::qam(16, single);
  const pluss::Series symbols = cm::modulate(c, random_bits(40, 1));
  out.writeln("symbol dtype is Complex64", symbols.scalar_type() == pluss::ScalarType::Complex64);
  out.writeln("noisy symbol dtype is Complex64", cm::noise(symbols, 0.01, 5).scalar_type() == pluss::ScalarType::Complex64);
  out.writeln("first symbol", flat(symbols.real())[0], flat(symbols.imag())[0]);
}
```

Output

```text
symbol dtype is Complex64 true
noisy symbol dtype is Complex64 true
first symbol -0.9486833 -0.3162278
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Constellation.html">
    <span class="label">Previous</span>
    <span class="title">Constellation</span>
  </a>
  <a class="next-link" href="psk.html">
    <span class="label">Next</span>
    <span class="title">psk</span>
  </a>
</div>
<!-- apiref:nav:end -->
