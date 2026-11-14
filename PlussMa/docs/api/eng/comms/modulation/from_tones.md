<!-- generated from from_tones.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::from_tones`

An OFDM waveform to subcarrier symbols.

## Signature

```cpp
pluss::Series comms::from_tones(const pluss::Series& signal, std::int64_t fft_size, std::int64_t cyclic_prefix);
```

## What It Does

Drops the cyclic prefix of every block and takes the unitary FFT of the rest: the inverse of `to_tones`.

## Parameters

`signal`
: A complex `[..., F * (fft_size + cyclic_prefix)]` waveform.

`fft_size`
: The number of subcarriers per block.

`cyclic_prefix`
: The prefix length in samples.

## Returns

A complex `[..., F * fft_size]` array of subcarrier symbols.

## Errors

Throws `pluss::Error` for a signal whose last length is not a whole number of blocks, or an invalid `fft_size` or `cyclic_prefix`.

## Interpretation and Use

After a channel with a short impulse response the result is `H[k] X[k]` for subcarrier `k`; divide by the channel's frequency response (`fft` of the zero-padded impulse response) to equalise.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A round trip.

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
  // 5 OFDM blocks of 16 subcarriers carrying 16-QAM, with a cyclic prefix of 4 samples
  const cm::Constellation c = cm::qam(16);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 16 * 5, 2));
  const pluss::Series waveform = cm::to_tones(symbols, 16, 4);
  out.writeln("symbols", symbols.sizes(), " waveform", waveform.sizes(), " = 5 * (16 + 4)");
  const pluss::Series back = cm::from_tones(waveform, 16, 4);
  out.writeln("round trip evm", cm::evm(back, symbols).item<double>());
  const pluss::Series body = waveform.reshape({5, 20}).slice(-1, 4, 20);
  out.writeln("energy per sample", body.abs().mul(body.abs()).mean().item<double>(), " energy per symbol", symbols.abs().mul(symbols.abs()).mean().item<double>());
  out.writeln("the prefix repeats the end of each block", waveform.slice(-1, 0, 4).sub(waveform.slice(-1, 16, 20)).abs().amax().item<double>() < 1e-12);
}
```

Output

```text
symbols [80]  waveform [100]  = 5 * (16 + 4)
round trip evm 0.000000
energy per sample 0.960000  energy per symbol 0.960000
the prefix repeats the end of each block true
```
### Example 2

A multipath channel and one-tap equalisation.

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
  // a four-tap multipath channel shorter than the prefix becomes one complex gain per subcarrier
  const std::int64_t k = 16, cp = 4, frames = 5;
  const cm::Constellation c = cm::qam(16);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * k * frames, 3));
  const pluss::Series tx = cm::to_tones(symbols, k, cp);
  const std::vector<double> h_re{1.0, 0.0, 0.4, 0.0}, h_im{0.0, 0.3, 0.0, -0.2};
  const pluss::Series taps = pluss::series::complex(vec(h_re), vec(h_im), pluss::ScalarType::Complex128);
  const pluss::Series rx = pluss::subsp::eng::signal::conv(tx, taps).slice(-1, 0, tx.size(-1));
  const pluss::Series received = cm::from_tones(rx, k, cp).reshape({frames, k});
  // the channel's frequency response on the subcarriers
  std::vector<double> pad_re = h_re, pad_im = h_im;
  pad_re.resize(k, 0.0);
  pad_im.resize(k, 0.0);
  const pluss::Series response = pluss::series::complex(vec(pad_re), vec(pad_im), pluss::ScalarType::Complex128).fft(k, -1, "backward");
  out.writeln("evm before equalisation", cm::evm(received, symbols.reshape({frames, k})).amax().item<double>(), "(a different gain on every subcarrier)");
  const pluss::Series equalised = received.div(response);
  out.writeln("evm after dividing by the channel response", cm::evm(equalised, symbols.reshape({frames, k})).amax().item<double>());
  // a prefix shorter than the channel memory leaves intersymbol interference
  const pluss::Series short_rx = cm::from_tones(pluss::subsp::eng::signal::conv(cm::to_tones(symbols, k, 1), taps).slice(-1, 0, frames * (k + 1)), k, 1).reshape({frames, k});
  out.writeln("with a one-sample prefix, evm after dividing", cm::evm(short_rx.div(response), symbols.reshape({frames, k})).amax().item<double>());
}
```

Output

```text
evm before equalisation 0.534684 (a different gain on every subcarrier)
evm after dividing by the channel response 0.000000
with a one-sample prefix, evm after dividing 0.756887
```
### Example 3

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
  const pluss::Series symbols = cm::modulate(cm::qam(16), random_bits(4 * 16, 1));
  report([&] { (void)cm::to_tones(symbols, 7, 1); });
  report([&] { (void)cm::to_tones(symbols, 16, 16); });
  report([&] { (void)cm::to_tones(random_bits(16, 1), 16, 4); });
  report([&] { (void)cm::from_tones(cm::to_tones(symbols, 16, 4).slice(-1, 0, 19), 16, 4); });
}
```

Output

```text
eng::comms::to_tones: symbols must be a complex [..., F * fft_size] array
eng::comms::to_tones: fft_size must be at least 2 and cyclic_prefix in [0, fft_size)
eng::comms::to_tones: symbols must be a complex [..., F * fft_size] array
eng::comms::from_tones: signal must be a complex [..., F * (fft_size + cyclic_prefix)] array
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_tones.html">
    <span class="label">Previous</span>
    <span class="title">to_tones</span>
  </a>
</div>
<!-- apiref:nav:end -->
