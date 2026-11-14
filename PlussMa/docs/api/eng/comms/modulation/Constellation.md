<!-- generated from Constellation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::Constellation`

A set of complex symbols with their bit labels.

## Signature

```cpp
comms::Constellation
```

## What It Does

The result of `psk` and `qam`: the points of a modulation and the bits each carries. `points[i]` is the symbol sent for the integer label `i`, whose binary digits (most significant first) are the `bits` bits it carries. The average symbol energy is one, so `1 / noise_variance` is the signal-to-noise ratio per symbol.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `points` | The complex symbol of each label, a complex vector `[order]`. |  |
| `bits` | The number of bits per symbol. |  |
| `order` | The number of points, `2^bits`. |  |
| `phase_shift_keying` | `true` for PSK and `false` for QAM; it selects the theory formula of `theory`. |  |

## Returns

Returned by `psk` and `qam`; read by every function of the branch.

## Errors

Functions that take a constellation throw `pluss::Error` when it is not one made by `psk` or `qam`.

## Interpretation and Use

Build a constellation once and reuse it. The average energy `E|s|^2` is one for every constellation, so Es/N0 is `1 / N0` and Eb/N0 is `1 / (bits N0)`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Fields of three constellations.

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
### Example 2

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
### Example 3

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="ConstellationOptions.html">
    <span class="label">Next</span>
    <span class="title">ConstellationOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
