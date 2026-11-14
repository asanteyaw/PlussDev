<!-- generated from gardner.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `comms::gardner`

Gardner symbol-timing recovery.

## Signature

```cpp
comms::TimingResult comms::gardner(const pluss::Series& filtered, std::int64_t samples_per_symbol, double loop_bandwidth);
```

## What It Does

Runs a second-order timing loop over a matched-filtered waveform. The sampling instant is interpolated (cubic) from the waveform; the Gardner error `Re{conj(y(mid)) (y(n) - y(n - 1))}` (normalised by the signal power) is positive when the instant is late; a proportional-plus-integral filter with damping `1 / sqrt(2)` moves the instant, so the loop also follows a sample-clock drift. The loop runs over the symbols and is vectorized over frames.

## Parameters

`filtered`
: The matched-filtered waveform, a real or complex `[..., L]` array holding at least eight symbols.

`samples_per_symbol`
: The oversampling factor, at least `2`.

`loop_bandwidth`
: The normalised noise bandwidth of the loop, in `(0, 0.5)`; about `0.005` to `0.03` is typical.

## Returns

A `TimingResult` with the recovered symbols and their sampling instants.

## Errors

Throws `pluss::Error` for `samples_per_symbol` below `2`, a `loop_bandwidth` outside `(0, 0.5)`, or a waveform that is not real or complex with at least eight symbols.

## Interpretation and Use

The error is independent of the symbols and of the carrier phase for a rotated constellation, but the loop assumes a pulse with a clean eye (a root-raised-cosine pair); too wide a loop passes noise and can lose lock. For a block with a known fixed offset `timing` is a one-shot alternative.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Locking onto three offsets.

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
### Example 2

The loop bandwidth.

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
  // a wider loop settles faster and tracks a larger timing drift but passes more noise
  const cm::Constellation qpsk = cm::psk(4);
  const pluss::Series taps = cm::rrc(0.35, 4, 12);
  const pluss::Series symbols = cm::modulate(qpsk, random_bits(2 * 1000, 7));
  const pluss::Series noisy = cm::noise(chain(symbols, taps, 1.3), 0.02, 9);
  out.writeln("loop bandwidth  best evm over the stream alignment after settling");
  for (double bandwidth : {0.005, 0.01, 0.03, 0.1}) {
    const cm::TimingResult got = cm::gardner(noisy, 4, bandwidth);
    double best = 1e9;
    for (int lag = 0; lag < 40; ++lag) best = std::min(best, cm::evm(got.symbols.slice(-1, 400 + lag, 900 + lag), symbols.slice(-1, 400, 900)).item<double>());
    out.writeln(bandwidth, best);
  }
}
```

Output

```text
loop bandwidth  best evm over the stream alignment after settling
0.0050 0.1196
0.0100 0.1205
0.0300 0.1297
0.1000 1.1952
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
  <a class="prev-link" href="TimingResult.html">
    <span class="label">Previous</span>
    <span class="title">TimingResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
