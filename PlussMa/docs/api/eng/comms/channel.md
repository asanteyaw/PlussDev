<!-- generated from channel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Channel

This branch models the channel. `noise` adds white Gaussian noise and `n0` converts an Eb/N0 into the noise variance to add.

## Choosing A Surface

Compute the variance from the Eb/N0 you want with `n0`, then add the noise with `noise`.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Noise of a given variance.

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
  const pluss::Series sent = cm::symbols(c, pluss::series::zeros({50000}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64)));
  const pluss::Series noise = cm::noise(sent, 0.2, 1).sub(sent);
  out.writeln("total noise variance E|n|^2", noise.abs().mul(noise.abs()).mean().item<double>(), " requested 0.2");
  out.writeln("real part", noise.real().mul(noise.real()).mean().item<double>(), " imaginary part", noise.imag().mul(noise.imag()).mean().item<double>(), " (each half)");
  out.writeln("zero variance adds nothing", cm::noise(sent, 0.0, 1).sub(sent).abs().amax().item<double>() == 0.0);
}
```

Output

```text
total noise variance E|n|^2 0.2010  requested 0.2
real part 0.1008  imaginary part 0.1002  (each half)
zero variance adds nothing true
```
### Example 2

Noise for an Eb/N0.

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
  out.writeln("N0 at Eb/N0 = 0 dB for BPSK, QPSK, 16-QAM, 64-QAM");
  for (const cm::Constellation &c : {cm::psk(2), cm::psk(4), cm::qam(16), cm::qam(64)}) out.writeln("order", c.order, cm::n0(0.0, c));
  // the same N0 is a larger Eb/N0 for a constellation with more bits per symbol: each bit carries less energy
  out.writeln("N0 at Eb/N0 = 10 dB for QPSK", cm::n0(10.0, cm::psk(4)), " and 10 log10 of 1 / (2 N0)", 10.0 * std::log10(1.0 / (2.0 * cm::n0(10.0, cm::psk(4)))));
}
```

Output

```text
N0 at Eb/N0 = 0 dB for BPSK, QPSK, 16-QAM, 64-QAM
order 2 1.00000
order 4 0.50000
order 16 0.25000
order 64 0.16667
N0 at Eb/N0 = 10 dB for QPSK 0.05000  and 10 log10 of 1 / (2 N0) 10.00000
```
### Example 3

Reproducible noise.

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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series sent = cm::modulate(c, random_bits(2000, 2));
  const pluss::Series a = cm::noise(sent, 0.1, 42), b = cm::noise(sent, 0.1, 42), other = cm::noise(sent, 0.1, 43);
  out.writeln("same seed, same noise", a.sub(b).abs().amax().item<double>() == 0.0);
  out.writeln("another seed, another noise", a.sub(other).abs().amax().item<double>() > 0.0);
}
```

Output

```text
same seed, same noise true
another seed, another noise true
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

channel/noise
channel/n0
```
<!-- apiref:toctree:end -->
