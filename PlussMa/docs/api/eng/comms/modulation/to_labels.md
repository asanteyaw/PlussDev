<!-- generated from to_labels.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::to_labels`

Bits to labels.

## Signature

```cpp
pluss::Series comms::to_labels(const pluss::Series& bits, std::int64_t bits_per_symbol);
```

## What It Does

Reads each group of `bits_per_symbol` bits as an integer label, the first bit being the most significant.

## Parameters

`bits`
: Zeros and ones, a real `[..., N]` array with `N` a multiple of `bits_per_symbol`.

`bits_per_symbol`
: The number of bits per label, at least `1`.

## Returns

An `Int64` array `[..., N / bits_per_symbol]`.

## Errors

Throws `pluss::Error` for `bits_per_symbol` below `1`, a bit count that is not a multiple of it, or values other than zero and one.

## Interpretation and Use

The inverse of `to_bits`. Floating zeros and ones are accepted.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Labels and bits round trip.

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
### Example 2

Batches and float bits.

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
  const pluss::Series bits = random_bits(2 * 12, 4).reshape({2, 12});
  const pluss::Series labels = cm::to_labels(bits, 4);
  out.writeln("bits", bits.sizes(), " labels", labels.sizes(), " bits again", cm::to_bits(labels, 4).sizes());
  out.writeln("round trip exact", cm::to_bits(labels, 4).sub(bits).abs().amax().item<double>() == 0.0);
  // floating zeros and ones are accepted as well
  out.writeln("float bits", labels_of(cm::to_labels(vec({1, 1, 0, 1}), 2)));
}
```

Output

```text
bits [2, 12]  labels [2, 3]  bits again [2, 12]
round trip exact true
float bits [3, 1]
```
### Example 3

Rejected bit arrays.

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
  report([&] { (void)cm::to_labels(random_bits(7, 1), 2); });
  report([&] { (void)cm::to_labels(vec({0, 1, 2, 1}), 2); });
  report([&] { (void)cm::to_labels(random_bits(4, 1), 0); });
  report([&] { (void)cm::to_bits(pluss::series::make(std::vector<std::int64_t>{4}), 2); });
  report([&] { (void)cm::to_bits(pluss::series::make(std::vector<std::int64_t>{-1}), 2); });
}
```

Output

```text
eng::comms::to_labels: the number of bits must be a multiple of bits_per_symbol
eng::comms::to_labels: bits must be zeros and ones
eng::comms::to_labels: bits_per_symbol must be at least 1
eng::comms::to_bits: labels must lie in [0, 2^bits_per_symbol)
eng::comms::to_bits: labels must lie in [0, 2^bits_per_symbol)
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_bits.html">
    <span class="label">Previous</span>
    <span class="title">to_bits</span>
  </a>
  <a class="next-link" href="detect.html">
    <span class="label">Next</span>
    <span class="title">detect</span>
  </a>
</div>
<!-- apiref:nav:end -->
