<!-- generated from constellations_and_mapping.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Constellations and mapping

[Back to Communications 1: Modulation and Detection](../comms_01_modulation.md)

A **constellation** is the set of complex numbers a transmitter may send, one per pattern of bits. This page builds phase-shift-keying (PSK) and square quadrature-amplitude (QAM) constellations, shows why the labelling of the points matters, and maps bits to symbols and back.

## The mental model

`comms::psk(order)` places `order` points on the unit circle; `comms::qam(order)` places `order` points on a square grid, `order` a power of four. Both return a `Constellation`: `points[label]` is the complex symbol sent for the integer `label`, the label's binary digits (most significant first) are the `bits` bits that symbol carries, and the average energy `E|s|^2` is **one**. That normalisation fixes the scale of everything that follows: a noise variance `N0` per symbol is then a symbol signal-to-noise ratio of `1 / N0`.

Two choices shape the labels.

- **Gray labelling** (the default): neighbouring points differ in exactly one bit, so the most likely error, a decision for a neighbouring point, flips only one bit.
- **Phase and precision:** `ConstellationOptions` rotates the points and picks `Complex128` or `Complex64`.

## A constellation and its fields

QPSK has four points on the unit circle. The program prints the constellation's order and bits and the real and imaginary parts by label, then a point of 8-PSK.

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

Four points carry two bits each, and the labels `0, 1, 2, 3` sit at angles `0, 90, 270, 180` degrees: the Gray order, in which labels `01` and `11` (not `01` and `10`) are neighbours.

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
  for (const cm::Constellation &c : {cm::psk(2), cm::psk(8), cm::qam(64)}) {
    out.writeln("order", c.order, " bits", c.bits, " psk", c.phase_shift_keying, " points shape", c.points.sizes(),
                " average energy", c.points.abs().mul(c.points.abs()).mean().item<double>());
  }
}
```

Output

```text
order 2  bits 1  psk true  points shape [2]  average energy 1.00000
order 8  bits 3  psk true  points shape [8]  average energy 1.00000
order 64  bits 6  psk false  points shape [64]  average energy 1.00000
```

Every constellation has unit average energy: BPSK has two points and one bit, 8-PSK eight and three, and 64-QAM sixty-four and six, with `psk` true only for the first two.

## Why Gray labelling

Walking once round the 8-PSK circle, the Gray labels `0, 1, 3, 2, 6, 7, 5, 4` change one bit between every pair of neighbours. Natural labels change one, two or three bits.

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

The Gray ring has exactly one bit change at every step, and the natural labels `0 ... 7` change `1, 2, 1, 3, 1, 2, 1, 3`: a symbol error to a neighbour costs up to three bit errors.

## Square QAM

16-QAM is a four-by-four grid scaled to unit energy: four levels `+-0.316` and `+-0.949` on each axis. The high two bits of the label choose the in-phase level and the low two the quadrature level.

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

The in-phase levels of labels `0, 4, 8, 12` are `-0.9487, -0.3162, 0.9487, 0.3162` (Gray order, not sorted) and the average energy is exactly one. The nearest-neighbour property also holds for the grid.

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

With Gray labels the closest pairs of points differ in one bit, with natural labels in up to two.

## Bits to symbols

`modulate` groups the bits `bits` at a time, reads each group as a label (the first bit is the most significant) and returns the symbol. The program maps eight bits to four QPSK symbols and the same number of bits to two 16-QAM symbols.

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
  // QPSK: two bits per symbol, the first bit is the high bit of the label
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{0, 0, 0, 1, 1, 1, 1, 0});
  const pluss::Series symbols = cm::modulate(qpsk, bits);
  out.writeln("bits", labels_of(bits));
  out.writeln("symbols", symbols.sizes(), " real", flat(symbols.real()), " imag", flat(symbols.imag()));
  // 16-QAM: four bits per symbol
  out.writeln("16-QAM symbols from 8 bits", cm::modulate(cm::qam(16), pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1, 0, 0, 1, 0})).sizes());
}
```

Output

```text
bits [0, 0, 0, 1, 1, 1, 1, 0]
symbols [4]  real [1.0000, 0.0000, -1.0000, -0.0000]  imag [0.0000, 1.0000, 0.0000, -1.0000]
16-QAM symbols from 8 bits [2]
```

The bits `00 01 11 10` become the points at `0, 90, 180` and `270` degrees. `to_labels` and `to_bits` convert between the two representations without a constellation.

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
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1});
  const pluss::Series labels = cm::to_labels(bits, 3);
  out.writeln("labels of 3 bits", labels_of(labels));
  out.writeln("back to bits   ", labels_of(cm::to_bits(labels, 3)));
  // the first bit of each group is the most significant
  out.writeln("label of 1,0,1", labels_of(cm::to_labels(pluss::series::make(std::vector<std::int64_t>{1, 0, 1}), 3)));
  out.writeln("bits of label 6 in 3 bits", labels_of(cm::to_bits(pluss::series::make(std::vector<std::int64_t>{6}), 3)));
}
```

Output

```text
labels of 3 bits [5, 5, 1, 7]
back to bits    [1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1]
label of 1,0,1 [5]
bits of label 6 in 3 bits [1, 1, 0]
```

Twelve bits in groups of three make the labels `5, 5, 1, 7`; the first bit of `1, 0, 1` is the most significant, so its label is five.

## Frames in a batch

Leading axes are independent frames, and every function of the branch is vectorized over them.

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
  // a leading axis is a batch of independent frames
  const cm::Constellation c = cm::qam(16);
  const pluss::Series bits = random_bits(3 * 80, 2).reshape({3, 80});
  const pluss::Series symbols = cm::modulate(c, bits);
  out.writeln("bits", bits.sizes(), " symbols", symbols.sizes(), " (frame, symbol)");
  out.writeln("round trip recovers every bit", cm::ber(bits, cm::demod(c, symbols)).amax().item<double>() == 0.0);
}
```

Output

```text
bits [3, 80]  symbols [3, 20]  (frame, symbol)
round trip recovers every bit true
```

## Errors

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
  const cm::Constellation c = cm::qam(16);
  report([&] { (void)cm::modulate(c, random_bits(7, 1)); });
  report([&] { (void)cm::modulate(c, vec({0, 1, 2, 1})); });
  report([&] { (void)cm::modulate(c, pluss::Series{}); });
  cm::Constellation broken;
  report([&] { (void)cm::modulate(broken, random_bits(8, 1)); });
}
```

Output

```text
eng::comms::to_labels: the number of bits must be a multiple of bits_per_symbol
eng::comms::to_labels: bits must be zeros and ones
eng::comms::to_labels: bits must be a real [..., N] array
eng::comms::modulate: the constellation is not one made by psk or qam
```

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

## Decisions

- **Phase modulation:** `psk`; BPSK and QPSK for robustness, 8-PSK and above for rate.
- **Amplitude and phase:** `qam`, order a power of four.
- **Anything that a decoder will see:** keep the Gray labelling (the default).
- **Single precision:** `ConstellationOptions::dtype = Complex64`; the symbols and the noise added later stay single.

## Pitfalls

The program makes three mistakes: sending a number of bits that is not a multiple of the bits per symbol, reading the first bit of a label as the least significant, and assuming Gray and natural constellations of one order place a label at the same point.

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation c = cm::qam(16);

  // 1. seven bits cannot make whole 16-QAM symbols
  try {
    (void)cm::modulate(c, random_bits(7, 1));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }

  // 2. the first bit is the most significant: 1,0,0,0 is label 8, not label 1
  const pluss::Series bits = pluss::series::make(std::vector<std::int64_t>{1, 0, 0, 0});
  out.writeln("label of the bits 1,0,0,0", cm::to_labels(bits, 4).items<std::int64_t>()[0]);

  // 3. natural and Gray constellations of the same order place a given label at different points
  cm::ConstellationOptions natural;
  natural.gray = false;
  const pluss::Series gray_point = cm::modulate(cm::psk(8), pluss::series::make(std::vector<std::int64_t>{1, 1, 0}));
  const pluss::Series natural_point = cm::modulate(cm::psk(8, natural), pluss::series::make(std::vector<std::int64_t>{1, 1, 0}));
  out.writeln("label 6 in Gray 8-PSK, angle in degrees   ", std::atan2(flat(gray_point.imag())[0], flat(gray_point.real())[0]) * 180.0 / M_PI);
  out.writeln("label 6 in natural 8-PSK, angle in degrees", std::atan2(flat(natural_point.imag())[0], flat(natural_point.real())[0]) * 180.0 / M_PI);
}
```

Output

```text
eng::comms::to_labels: the number of bits must be a multiple of bits_per_symbol
label of the bits 1,0,0,0 8
label 6 in Gray 8-PSK, angle in degrees    180.0000
label 6 in natural 8-PSK, angle in degrees -90.0000
```

Seven bits cannot make whole 16-QAM symbols, and the message says so. The bits `1, 0, 0, 0` are label `8`, not `1`. And label `6` sits at `180` degrees in Gray 8-PSK and at `-90` degrees in natural 8-PSK, so the two constellations must not be mixed between transmitter and receiver.

## Where next

[Noise and detection](noise_and_detection.md) sends symbols through a noisy channel and decides what was sent. Reference pages: [`psk`](../../../../api/eng/comms/modulation/psk.md), [`qam`](../../../../api/eng/comms/modulation/qam.md), [`Constellation`](../../../../api/eng/comms/modulation/Constellation.md), [`modulate`](../../../../api/eng/comms/modulation/modulate.md), [`ConstellationOptions`](../../../../api/eng/comms/modulation/ConstellationOptions.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="noise_and_detection.html">
    <span class="label">Next</span>
    <span class="title">Noise and detection</span>
  </a>
</div>
<!-- apiref:nav:end -->
