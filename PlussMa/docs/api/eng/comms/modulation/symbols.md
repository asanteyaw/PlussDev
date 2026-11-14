<!-- generated from symbols.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::symbols`

Labels to symbols.

## Signature

```cpp
pluss::Series comms::symbols(const comms::Constellation& constellation, const pluss::Series& labels);
```

## What It Does

Looks up the point of each integer label. It is `modulate` without the bit packing.

## Parameters

`constellation`
: The constellation to map to.

`labels`
: Integer labels in `[0, order)`, a real array of any shape with at least one axis.

## Returns

A complex array of the same shape as `labels`.

## Errors

Throws `pluss::Error` for a constellation not made by `psk` or `qam`, labels outside `[0, order)`, or an undefined array.

## Interpretation and Use

Use `symbols` when the data is already symbol indices, and as the reference against which `detect` is checked.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Labels to points, and a matrix of labels.

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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series labels = pluss::series::make(std::vector<std::int64_t>{0, 1, 2, 3, 3, 0});
  const pluss::Series symbols = cm::symbols(c, labels);
  out.writeln("real", flat(symbols.real()));
  out.writeln("imag", flat(symbols.imag()));
  // a matrix of labels keeps its shape
  const pluss::Series grid = pluss::series::make(std::vector<std::int64_t>{0, 1, 2, 3}).reshape({2, 2});
  out.writeln("symbol shape", cm::symbols(c, grid).sizes());
}
```

Output

```text
real [1.0000, 0.0000, -0.0000, -1.0000, -1.0000, 1.0000]
imag [0.0000, 1.0000, -1.0000, 0.0000, 0.0000, 0.0000]
symbol shape [2, 2]
```
### Example 2

Bits to symbols for comparison.

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
### Example 3

Rejected labels.

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
  report([&] { (void)cm::symbols(c, pluss::series::make(std::vector<std::int64_t>{0, 4})); });
  report([&] { (void)cm::symbols(c, pluss::series::make(std::vector<std::int64_t>{-1})); });
  report([&] { (void)cm::symbols(c, pluss::Series{}); });
}
```

Output

```text
eng::comms::symbols: labels must lie in [0, order)
eng::comms::symbols: labels must lie in [0, order)
eng::comms::symbols: labels must be a real integer array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="modulate.html">
    <span class="label">Previous</span>
    <span class="title">modulate</span>
  </a>
  <a class="next-link" href="to_bits.html">
    <span class="label">Next</span>
    <span class="title">to_bits</span>
  </a>
</div>
<!-- apiref:nav:end -->
