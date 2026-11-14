<!-- generated from evm.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::evm`

The error vector magnitude.

## Signature

```cpp
pluss::Series comms::evm(const pluss::Series& received, const pluss::Series& reference);
```

## What It Does

Returns the root-mean-square distance between the received and reference symbols divided by the root-mean-square of the reference, over the last axis.

## Parameters

`received`
: The received symbols, `[..., N]`.

`reference`
: The ideal symbols, of the same shape.

## Returns

A real array of shape `[...]`; zero when the received symbols equal the reference.

## Errors

Throws `pluss::Error` for arrays of different shapes or an undefined array.

## Interpretation and Use

For unit-energy symbols in noise of variance `N0` the result is `sqrt(N0)`. It reads gain and phase errors as well as noise, so it is a good quality measure of a transmitter or of a processing chain.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

EVM against the noise level.

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
  // for unit-energy symbols in noise of variance N0 the error vector magnitude is sqrt(N0)
  const cm::Constellation c = cm::qam(16);
  const pluss::Series sent = cm::modulate(c, random_bits(4 * 20000, 7));
  for (double n0 : {0.01, 0.04, 0.16}) {
    const pluss::Series received = cm::noise(sent, n0, 3);
    out.writeln("N0", n0, " evm", cm::evm(received, sent).item<double>(), " sqrt(N0)", std::sqrt(n0));
  }
}
```

Output

```text
N0 0.0100  evm 0.1004  sqrt(N0) 0.1000
N0 0.0400  evm 0.2008  sqrt(N0) 0.2000
N0 0.1600  evm 0.4016  sqrt(N0) 0.4000
```
### Example 2

Frames, and a gain error.

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
  const pluss::Series sent = cm::modulate(c, random_bits(3 * 2 * 2000, 5).reshape({3, 4000}));
  const pluss::Series received = cm::noise(sent, 0.09, 8);
  out.writeln("evm of each frame", flat(cm::evm(received, sent)), " expected about 0.3");
  out.writeln("a gain error of 10 percent shows as an evm of", cm::evm(sent.mul(1.1), sent).amax().item<double>());
  out.writeln("noise-free", cm::evm(sent, sent).amax().item<double>());
}
```

Output

```text
evm of each frame [0.2995, 0.2987, 0.2987]  expected about 0.3
a gain error of 10 percent shows as an evm of 0.1000
noise-free 0.0000
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
  const pluss::Series sent = cm::modulate(cm::psk(4), random_bits(8, 1));
  report([&] { (void)cm::evm(sent, sent.slice(0, 0, 3)); });
  report([&] { (void)cm::evm(pluss::Series{}, sent); });
}
```

Output

```text
eng::comms::evm: received and reference must have the same shape [..., N]
eng::comms::evm: received and reference must have the same shape [..., N]
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ser.html">
    <span class="label">Previous</span>
    <span class="title">ser</span>
  </a>
  <a class="next-link" href="theory.html">
    <span class="label">Next</span>
    <span class="title">theory</span>
  </a>
</div>
<!-- apiref:nav:end -->
