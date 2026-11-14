<!-- generated from ber.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::ber`

The fraction of bits that differ.

## Signature

```cpp
pluss::Series comms::ber(const pluss::Series& sent, const pluss::Series& received);
```

## What It Does

Returns the fraction of positions at which `sent` and `received` differ, over the last axis.

## Parameters

`sent`
: The transmitted bits, `[..., N]`.

`received`
: The decided bits, of the same shape.

## Returns

A real array of shape `[...]`; zero for identical streams.

## Errors

Throws `pluss::Error` for arrays of different shapes or an empty last axis.

## Interpretation and Use

Values compare by value, so integer and floating bits can be mixed. A bit error rate estimated from `K` errors has a relative uncertainty of about `1 / sqrt(K)`: count enough errors before trusting a digit.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two of eight bits differ.

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
  const pluss::Series sent = pluss::series::make(std::vector<std::int64_t>{0, 1, 1, 0, 1, 0, 0, 1});
  const pluss::Series received = pluss::series::make(std::vector<std::int64_t>{0, 1, 0, 0, 1, 1, 0, 1});
  out.writeln("two of eight bits differ", cm::ber(sent, received).item<double>());
  out.writeln("identical streams", cm::ber(sent, sent).item<double>());
  out.writeln("float and integer bits compare by value", cm::ber(vec({0, 1, 1, 0}), pluss::series::make(std::vector<std::int64_t>{0, 1, 1, 1})).item<double>());
}
```

Output

```text
two of eight bits differ 0.25
identical streams 0
float and integer bits compare by value 0.25
```
### Example 2

One rate per frame.

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
  // one error rate per frame
  const cm::Constellation c = cm::psk(4);
  const pluss::Series bits = random_bits(3 * 2000, 2).reshape({3, 2000});
  const pluss::Series symbols = cm::modulate(c, bits);
  for (double ebn0 : {0.0, 4.0, 8.0}) {
    const pluss::Series decided = cm::demod(c, cm::noise(symbols, cm::n0(ebn0, c), 13));
    out.writeln("Eb/N0", ebn0, "dB  error rate of each frame", flat(cm::ber(bits, decided)));
  }
}
```

Output

```text
Eb/N0 0.0000 dB  error rate of each frame [0.0840, 0.0875, 0.0940]
Eb/N0 4.0000 dB  error rate of each frame [0.0130, 0.0160, 0.0160]
Eb/N0 8.0000 dB  error rate of each frame [0.0000, 0.0005, 0.0000]
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
  <a class="next-link" href="ser.html">
    <span class="label">Next</span>
    <span class="title">ser</span>
  </a>
</div>
<!-- apiref:nav:end -->
