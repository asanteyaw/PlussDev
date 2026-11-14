<!-- generated from ser.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::ser`

The fraction of symbols that differ.

## Signature

```cpp
pluss::Series comms::ser(const pluss::Series& sent, const pluss::Series& received);
```

## What It Does

Returns the fraction of positions at which two label arrays differ, over the last axis.

## Parameters

`sent`
: The transmitted labels, `[..., N]`.

`received`
: The decided labels from `detect`, of the same shape.

## Returns

A real array of shape `[...]`.

## Errors

Throws `pluss::Error` for arrays of different shapes or an empty last axis.

## Interpretation and Use

With Gray labels a symbol error is mostly a single bit error, so the bit error rate is close to the symbol error rate divided by the bits per symbol.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

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
### Example 2

A batch of detected frames.

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

Rejected arrays.

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
  report([&] { (void)cm::ber(vec({0, 1, 1}), vec({0, 1})); });
  report([&] { (void)cm::ber(vec({}), vec({})); });
  report([&] { (void)cm::ber(pluss::Series{}, vec({0, 1})); });
  report([&] { (void)cm::ser(vec({0, 1, 2}), vec({0, 1, 2, 3})); });
}
```

Output

```text
eng::comms::ber: both arrays must have the same shape [..., N]
eng::comms::ber: the last axis must not be empty
eng::comms::ber: both arrays must have the same shape [..., N]
eng::comms::ser: both arrays must have the same shape [..., N]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ber.html">
    <span class="label">Previous</span>
    <span class="title">ber</span>
  </a>
  <a class="next-link" href="evm.html">
    <span class="label">Next</span>
    <span class="title">evm</span>
  </a>
</div>
<!-- apiref:nav:end -->
