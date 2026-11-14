<!-- generated from matched.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::matched`

The matched filter without sampling.

## Signature

```cpp
pluss::Series comms::matched(const pluss::Series& waveform, const pluss::Series& taps);
```

## What It Does

Filters the waveform with the time-reversed taps (a full convolution): the receive side of a pulse-shaped link before any sampling. `match` is this followed by sampling at the symbol instants; `gardner` takes its output.

## Parameters

`waveform`
: A real or complex `[..., samples]` array.

`taps`
: The pulse the transmitter used, a real vector.

## Returns

The filtered waveform, `[..., samples + taps - 1]`.

## Errors

Throws `pluss::Error` for an undefined waveform or taps that are not a real vector.

## Interpretation and Use

The peak of the filtered pulse of the first symbol is at `taps - 1` samples (plus any channel delay). Use it when the sampling instants are not known in advance.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Filtering and sampling.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const cm::Constellation c = cm::qam(16);
  const pluss::Series taps = cm::rrc(0.35, 8, 10);
  const pluss::Series symbols = cm::modulate(c, random_bits(4 * 50, 4));
  const pluss::Series wave = cm::shape(symbols, taps, 8);
  // the matched filter is a convolution with the reversed taps: its length is the waveform plus the taps minus one
  const pluss::Series filtered = cm::matched(wave, taps);
  out.writeln("waveform", wave.sizes(), " taps", taps.numel(), " filtered", filtered.sizes());
  // sampling it at the combined delay (taps - 1) and every symbol period is what match does
  const std::int64_t delay = taps.numel() - 1;
  out.writeln("sampled filtered output equals match", filtered.select(-1, delay).sub(cm::match(wave, taps, 8, 50).select(-1, 0)).abs().item<double>() < 1e-12);
  out.writeln("peak of the filtered pulse of the first symbol is at the delay", flat(filtered.abs().slice(-1, delay - 2, delay + 3)));
  // the filtered waveform of a batch keeps its frames
  const pluss::Series frames = pluss::series::zeros({2, wave.size(-1)}, pluss::Config{}.with_dtype(pluss::ScalarType::Complex128)).add(wave);
  out.writeln("batched filtered shape", cm::matched(frames, taps).sizes());
}
```

Output

```text
waveform [473]  taps 81  filtered [553]
sampled filtered output equals match true
peak of the filtered pulse of the first symbol is at the delay [0.99975, 1.04001, 1.00088, 0.88084, 0.68848]
batched filtered shape [2, 553]
```
### Example 2

Feeding the timing loop.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace cm = pluss::subsp::eng::comms;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }
static pluss::Series chain(const pluss::Series &symbols, const pluss::Series &taps, double delay_samples) {
  return cm::matched(cm::delay(cm::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{delay_samples}).select(0, 0)), taps);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series symbols = cm::modulate(qpsk, random_bits(2 * 800, 91));
  // three unknown timing offsets: the loop finds the sampling instants by itself
  out.writeln("true delay  recovered symbols  best evm over the stream alignment");
  for (double delay : {0.0, 1.7, -1.1}) {
    const cm::TimingResult got = cm::gardner(chain(symbols, taps, delay), 4, 0.02);
    double best = 1e9;
    for (int lag = 0; lag < 30; ++lag) best = std::min(best, cm::evm(got.symbols.slice(-1, 200 + lag, 700 + lag), symbols.slice(-1, 200, 700)).item<double>());
    out.writeln(delay, got.symbols.sizes(), best);
  }
  // the sampling instants advance by one symbol (4 samples) per symbol once the loop has settled
  const cm::TimingResult one = cm::gardner(chain(symbols, taps, 1.7), 4, 0.02);
  const std::vector<double> instants = flat(one.offset);
  out.writeln("sampling instants of symbols 600 to 604", instants[600], instants[601], instants[602], instants[603], instants[604]);
  out.writeln("offsets shape", one.offset.sizes(), " symbols shape", one.symbols.sizes());
}
```

Output

```text
true delay  recovered symbols  best evm over the stream alignment
0.0000 [820] 0.0300
1.7000 [820] 0.0300
-1.1000 [820] 0.0298
sampling instants of symbols 600 to 604 2405.6713 2409.6291 2413.6382 2417.6676 2421.7056
offsets shape [820]  symbols shape [820]
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
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static pluss::Series random_bits(std::int64_t n, std::uint64_t seed) {
  return pluss::series::randn({n}, pluss::ScalarType::Float64, seed).gt(0.0).to(pluss::ScalarType::Int64);
}
static std::vector<std::int64_t> ints_of(const pluss::Series &s) { return s.reshape({s.numel()}).items<std::int64_t>(); }
static pluss::Series chain(const pluss::Series &symbols, const pluss::Series &taps, double delay_samples) {
  return cm::matched(cm::delay(cm::shape(symbols, taps, 4), pluss::series::make(std::vector<double>{delay_samples}).select(0, 0)), taps);
}

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series wave = chain(cm::modulate(cm::psk(4), random_bits(2 * 100, 1)), taps, 0.0);
  report([&] { (void)cm::gardner(wave, 1, 0.02); });
  report([&] { (void)cm::gardner(wave, 4, 0.0); });
  report([&] { (void)cm::gardner(wave, 4, 0.7); });
  report([&] { (void)cm::gardner(wave.slice(-1, 0, 20), 4, 0.02); });
  report([&] { (void)cm::matched(pluss::Series{}, taps); });
  report([&] { (void)cm::matched(wave, vec({})); });
}
```

Output

```text
eng::comms::gardner: samples_per_symbol must be at least 2
eng::comms::gardner: loop_bandwidth must lie in (0, 0.5)
eng::comms::gardner: loop_bandwidth must lie in (0, 0.5)
eng::comms::gardner: filtered must be a real or complex [..., L] array holding at least eight symbols
eng::comms::matched: waveform must be a [..., samples] array
eng::comms::matched: taps must be a real vector
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pll.html">
    <span class="label">Previous</span>
    <span class="title">pll</span>
  </a>
  <a class="next-link" href="TimingResult.html">
    <span class="label">Next</span>
    <span class="title">TimingResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
