<!-- generated from detect.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::detect`

Nearest-point symbol decisions.

## Signature

```cpp
pluss::Series comms::detect(const comms::Constellation& constellation, const pluss::Series& received);
```

## What It Does

Returns, for every received symbol, the label of the constellation point at the smallest Euclidean distance. This is the maximum-likelihood decision in white Gaussian noise when the symbols are equally likely.

## Parameters

`constellation`
: The constellation the symbols were sent from.

`received`
: The received complex symbols, a `[..., N]` array.

## Returns

An `Int64` array of labels with the shape of `received`.

## Errors

Throws `pluss::Error` for a constellation not made by `psk` or `qam` or an undefined or scalar `received`.

## Interpretation and Use

The call builds a `[..., N, order]` array of distances, so very large blocks of high-order constellations are worth splitting into frames. Compare with the sent labels using `ser`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Noisy decisions and a far-out symbol.

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
  const pluss::Series sent = pluss::series::make(std::vector<std::int64_t>{0, 5, 10, 15, 3, 12});
  const pluss::Series noisy = cm::noise(cm::symbols(c, sent), 0.005, 3);
  out.writeln("sent    ", labels_of(sent));
  out.writeln("detected", labels_of(cm::detect(c, noisy)));
  // a received symbol exactly between two points goes to the closer one; far outside the constellation to the nearest corner
  const pluss::Series far = pluss::series::complex(vec({9.0}), vec({9.0}), pluss::ScalarType::Complex128);
  out.writeln("far outside, up and right", labels_of(cm::detect(c, far)));
}
```

Output

```text
sent     [0, 5, 10, 15, 3, 12]
detected [0, 5, 10, 15, 3, 12]
far outside, up and right [10]
```
### Example 2

A batch of 8-PSK frames.

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
  const cm::Constellation c = cm::psk(8);
  const pluss::Series labels = pluss::series::randn({4, 100}, pluss::ScalarType::Float64, 7).abs().mul(1.0e3).floor().to(pluss::ScalarType::Int64);
  const pluss::Series sent = labels.sub(labels.div(8).floor().mul(8)).to(pluss::ScalarType::Int64);
  const pluss::Series detected = cm::detect(c, cm::symbols(c, sent));
  out.writeln("sent", sent.sizes(), " detected", detected.sizes(), " dtype is Int64", detected.scalar_type() == pluss::ScalarType::Int64);
  out.writeln("every label recovered", cm::ser(sent, detected).amax().item<double>() == 0.0);
}
```

Output

```text
sent [4, 100]  detected [4, 100]  dtype is Int64 true
every label recovered true
```
### Example 3

Symbol and bit error rates.

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
  const cm::Constellation c = cm::qam(16);
  const pluss::Series labels = pluss::series::randn({20000}, pluss::ScalarType::Float64, 4).abs().mul(1.0e3).floor();
  const pluss::Series sent = labels.sub(labels.div(16).floor().mul(16)).to(pluss::ScalarType::Int64);
  const pluss::Series received = cm::noise(cm::symbols(c, sent), 0.05, 5);
  const pluss::Series bits_sent = cm::to_bits(sent, 4), bits_decided = cm::demod(c, received);
  const double ser = cm::ser(sent, cm::detect(c, received)).item<double>();
  out.writeln("symbol error rate", ser, " bit error rate", cm::ber(bits_sent, bits_decided).item<double>());
  out.writeln("with Gray labels a symbol error is mostly one bit: ber is about ser / 4", ser / 4.0);
}
```

Output

```text
symbol error rate 0.06930  bit error rate 0.01760
with Gray labels a symbol error is mostly one bit: ber is about ser / 4 0.01733
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_labels.html">
    <span class="label">Previous</span>
    <span class="title">to_labels</span>
  </a>
  <a class="next-link" href="demod.html">
    <span class="label">Next</span>
    <span class="title">demod</span>
  </a>
</div>
<!-- apiref:nav:end -->
