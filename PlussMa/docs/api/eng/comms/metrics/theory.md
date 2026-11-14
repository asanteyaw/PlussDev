<!-- generated from theory.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::theory`

The theoretical bit error probability.

## Signature

```cpp
pluss::Series comms::theory(const comms::Constellation& constellation, const pluss::Series& ebn0_db);
```

## What It Does

Evaluates the bit error probability of Gray-coded coherent detection in white Gaussian noise: `Q(sqrt(2 Eb/N0))` for BPSK and QPSK (exact), `(2 / k) Q(sqrt(2 k Eb/N0) sin(pi / M))` for higher-order PSK, and `(4 / k) (1 - 1 / sqrt(M)) Q(sqrt(3 k Eb/N0 / (M - 1)))` for square QAM, where `k` is the bits per symbol and `M` the order.

## Parameters

`constellation`
: The constellation.

`ebn0_db`
: Eb/N0 in decibels, a real floating array of any shape.

## Returns

A real array of the shape of `ebn0_db`.

## Errors

Throws `pluss::Error` for a constellation not made by `psk` or `qam` or an `ebn0_db` that is not a real floating array.

## Interpretation and Use

The higher-order formulas are the nearest-neighbour approximation, accurate when the error rate is small, a few percent in the range the examples simulate. Use it as the reference line of a simulation.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Simulation against theory.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  struct Row { cm::Constellation c; const char *name; double ebn0; };
  const std::vector<Row> rows = {{cm::psk(2), "BPSK  ", 4.0}, {cm::psk(4), "QPSK  ", 6.0}, {cm::qam(16), "16-QAM", 8.0}, {cm::qam(64), "64-QAM", 12.0}};
  out.writeln("             Eb/N0   simulated    theory");
  std::uint64_t seed = 1;
  for (const Row &row : rows) {
    const pluss::Series bits = random_bits(row.c.bits * 100000, seed++);
    const pluss::Series received = cm::noise(cm::modulate(row.c, bits), cm::n0(row.ebn0, row.c), seed++);
    out.writeln(row.name, row.ebn0, cm::ber(bits, cm::demod(row.c, received)).item<double>(), cm::theory(row.c, vec({row.ebn0})).items<double>()[0]);
  }
}
```

Output

```text
             Eb/N0   simulated    theory
BPSK   4.000000 0.012140 0.012501
QPSK   6.000000 0.002415 0.002388
16-QAM 8.000000 0.009338 0.009247
64-QAM 12.000000 0.009778 0.009724
```
### Example 2

A waterfall in one call.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  // the whole waterfall in one call: a vector of Eb/N0 values gives a vector of probabilities
  const pluss::Series ebn0 = vec({0, 2, 4, 6, 8, 10, 12});
  out.writeln("Eb/N0 dB", flat(ebn0));
  out.writeln("BPSK  ", flat(cm::theory(cm::psk(2), ebn0)));
  out.writeln("16-QAM", flat(cm::theory(cm::qam(16), ebn0)));
  // a matrix of Eb/N0 values keeps its shape
  out.writeln("shape of a 2 x 2 input", cm::theory(cm::psk(4), vec({0, 4, 8, 12}).reshape({2, 2})).sizes());
}
```

Output

```text
Eb/N0 dB [0.00000000, 2.00000000, 4.00000000, 6.00000000, 8.00000000, 10.00000000, 12.00000000]
BPSK   [0.07864960, 0.03750613, 0.01250082, 0.00238829, 0.00019091, 0.00000387, 0.00000001]
16-QAM [0.13916001, 0.09755935, 0.05861846, 0.02787131, 0.00924721, 0.00175415, 0.00013866]
shape of a 2 x 2 input [2, 2]
```
### Example 3

Phase shift keying of increasing order.

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
  // QPSK has the same bit error probability as BPSK at the same Eb/N0; 8-PSK needs more energy per bit
  const pluss::Series ebn0 = vec({6.0, 10.0});
  out.print_defaults(PrintOptions{}.fixed(8));
  out.writeln("BPSK ", flat(cm::theory(cm::psk(2), ebn0)));
  out.writeln("QPSK ", flat(cm::theory(cm::psk(4), ebn0)));
  out.writeln("8-PSK", flat(cm::theory(cm::psk(8), ebn0)));
  out.writeln("16-PSK", flat(cm::theory(cm::psk(16), ebn0)));
}
```

Output

```text
BPSK  [0.00238829, 0.00000387]
QPSK  [0.00238829, 0.00000387]
8-PSK [0.02047992, 0.00101140]
16-PSK [0.06772598, 0.02024879]
```
### Example 4

Rejected arguments.

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
  report([&] { (void)cm::theory(cm::psk(4), pluss::Series{}); });
  report([&] { (void)cm::theory(cm::psk(4), pluss::series::make(std::vector<std::int64_t>{4, 8})); });
  report([&] { (void)cm::theory(cm::Constellation{}, vec({4.0})); });
}
```

Output

```text
eng::comms::theory: ebn0_db must be a real floating array
eng::comms::theory: ebn0_db must be a real floating array
eng::comms::theory: the constellation is not one made by psk or qam
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="evm.html">
    <span class="label">Previous</span>
    <span class="title">evm</span>
  </a>
</div>
<!-- apiref:nav:end -->
