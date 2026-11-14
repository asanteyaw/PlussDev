<!-- generated from noise.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::noise`

Additive white Gaussian noise.

## Signature

```cpp
pluss::Series comms::noise(const pluss::Series& signal, double noise_variance, std::uint64_t seed);
```

## What It Does

Adds zero-mean Gaussian noise of total variance `noise_variance` to every sample. For a complex signal the variance is split equally between the real and imaginary parts, so `E|n|^2 = noise_variance`; for a real signal it is the variance of the noise.

## Parameters

`signal`
: A real floating or complex array of any shape.

`noise_variance`
: The variance, non-negative; zero adds nothing.

`seed`
: The seed of the noise generator; the same seed gives the same noise.

## Returns

An array of the shape and dtype of `signal`.

## Errors

Throws `pluss::Error` for an undefined or integer `signal` or a negative or non-finite `noise_variance`.

## Interpretation and Use

Use `n0` to turn an Eb/N0 into the variance. Noise is reproducible from the seed; use different seeds for independent runs.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The variance and its split.

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

Seeds give reproducible noise.

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
### Example 3

Real signals.

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
  // a real signal receives real noise of the full variance
  const pluss::Series signal = pluss::series::zeros({40000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const pluss::Series noisy = cm::noise(signal, 0.5, 6);
  out.writeln("dtype stays real", !noisy.dtype().is_complex());
  out.writeln("variance", noisy.mul(noisy).mean().item<double>(), " requested 0.5");
  out.writeln("mean", noisy.mean().item<double>());
}
```

Output

```text
dtype stays real true
variance 0.4996  requested 0.5
mean 0.0033
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
  const pluss::Series sent = cm::symbols(cm::psk(4), pluss::series::make(std::vector<std::int64_t>{0, 1}));
  report([&] { (void)cm::noise(sent, -0.1, 1); });
  report([&] { (void)cm::noise(sent, std::nan(""), 1); });
  report([&] { (void)cm::noise(pluss::Series{}, 0.1, 1); });
  report([&] { (void)cm::noise(pluss::series::make(std::vector<std::int64_t>{1, 2}), 0.1, 1); });
}
```

Output

```text
eng::comms::noise: noise_variance must be non-negative
eng::comms::noise: noise_variance must be non-negative
eng::comms::noise: signal must be defined
eng::comms::noise: signal must be floating or complex
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="n0.html">
    <span class="label">Next</span>
    <span class="title">n0</span>
  </a>
</div>
<!-- apiref:nav:end -->
