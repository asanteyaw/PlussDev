<!-- generated from noise_and_detection.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Noise and detection

[Back to Communications 1: Modulation and Detection](../comms_01_modulation.md)

A channel adds noise, and the receiver has to decide what was sent. This page adds noise at a chosen energy per bit, makes hard decisions and soft log-likelihood ratios, counts the errors, and compares the measured error rate with theory for BPSK, QPSK, 16-QAM and 64-QAM.

## The mental model

With unit-energy symbols the noise is set by `N0`, the complex noise variance per symbol, and the quality of a link is the energy per bit to noise ratio `Eb/N0`. `n0(ebn0_db, constellation)` converts one into the other: `N0 = 1 / (bits 10^(Eb/N0 / 10))`, smaller for more bits per symbol at the same `Eb/N0`. `noise(signal, noise_variance, seed)` adds that noise, reproducibly from the seed.

The receiver then decides. `detect` returns the label of the nearest point, `demod` expands it to bits (hard decisions), and `llr` gives for every bit the **log-likelihood ratio** `log P(bit = 0) / P(bit = 1)` (positive favours zero), whose size is the confidence. The metrics `ber`, `ser` and `evm` measure the result, and `theory` is the probability to compare against.

## Noise at a chosen Eb/N0

For a fixed `Eb/N0` the noise variance depends on how many bits each symbol carries.

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

At `Eb/N0 = 0 dB` the noise variances are `1`, `0.5`, `0.25` and `0.167` for BPSK, QPSK, 16-QAM and 64-QAM: more bits per symbol means less energy per bit, and so less noise for the same ratio of the two.

The noise it adds has the variance asked for, split equally between the real and imaginary parts, and the same seed always gives the same noise.

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

## Hard decisions and the error rate

The program sends `80000` bits of 16-QAM at three values of `Eb/N0` and counts the errors.

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
  const pluss::Series bits = random_bits(4 * 20000, 9);
  const pluss::Series symbols = cm::modulate(c, bits);
  for (double ebn0 : {4.0, 8.0, 12.0}) {
    const pluss::Series received = cm::noise(symbols, cm::n0(ebn0, c), 31);
    out.writeln("Eb/N0", ebn0, "dB  bit error rate", cm::ber(bits, cm::demod(c, received)).item<double>());
  }
}
```

Output

```text
Eb/N0 4.00000 dB  bit error rate 0.05809
Eb/N0 8.00000 dB  bit error rate 0.00976
Eb/N0 12.00000 dB  bit error rate 0.00021
```

The error rate falls from `5.8 %` at `4 dB` to `1.0 %` at `8 dB` and `0.02 %` at `12 dB`: a factor of about six over the first four decibels and about fifty over the next.

`detect` returns the labels instead of bits, which is what to compare against the sent labels for the **symbol** error rate. With Gray labels a symbol error is mostly one bit error, so the bit error rate is close to the symbol error rate divided by the bits per symbol.

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

The symbol error rate is `6.9 %` and the bit error rate `1.8 %`, about a quarter, as predicted.

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

## Soft decisions

A hard decision throws away how close the symbol was to the boundary. The log-likelihood ratio keeps it. For BPSK it is `4 r / N0`; the program checks that and shows how a larger noise variance shrinks the confidence.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  // BPSK: label 0 is +1 and label 1 is -1 for the Gray-labelled constellation; the log-likelihood ratio is 4 r / N0
  const cm::Constellation bpsk = cm::psk(2);
  const std::vector<double> points = flat(bpsk.points.real());
  out.writeln("point of label 0 and label 1", points[0], points[1]);
  const pluss::Series received = pluss::series::complex(vec({0.9, 0.1, -0.1, -0.9}), vec({0.0, 0.0, 0.0, 0.0}), pluss::ScalarType::Complex128);
  out.writeln("llr at N0 = 0.5", flat(cm::llr(bpsk, received, 0.5)));
  out.writeln("4 r / N0       ", 4 * 0.9 / 0.5, 4 * 0.1 / 0.5, -4 * 0.1 / 0.5, -4 * 0.9 / 0.5);
  out.writeln("positive favours bit 0, and a larger N0 shrinks the confidence", flat(cm::llr(bpsk, received, 2.0)));
}
```

Output

```text
point of label 0 and label 1 1.000 -1.000
llr at N0 = 0.5 [7.200, 0.800, -0.800, -7.200]
4 r / N0        7.200 0.800 -0.800 -7.200
positive favours bit 0, and a larger N0 shrinks the confidence [1.800, 0.200, -0.200, -1.800]
```

The ratios `7.2, 0.8, -0.8, -7.2` equal `4 r / N0` at `N0 = 0.5`, and at `N0 = 2` they are a quarter the size. The sign is the hard decision.

For 16-QAM the sign agrees with the hard decision, and the size is a usable confidence: bits with a large ratio are almost never wrong.

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
  const pluss::Series bits = random_bits(4 * 5000, 12);
  const double n0 = cm::n0(8.0, c);
  const pluss::Series received = cm::noise(cm::modulate(c, bits), n0, 17);
  const pluss::Series soft = cm::llr(c, received, n0);
  const pluss::Series hard = cm::demod(c, received);
  out.writeln("llr shape", soft.sizes());
  out.writeln("sign of the llr agrees with the hard decision for all but", cm::ber(hard, soft.lt(0.0).to(pluss::ScalarType::Int64)).item<double>(), "of the bits");
  // the least confident bits are the ones most likely to be wrong
  const pluss::Series wrong = hard.ne(bits).to(pluss::ScalarType::Float64);
  const pluss::Series confident = soft.abs().gt(8.0).to(pluss::ScalarType::Float64);
  out.writeln("error rate overall", wrong.mean().item<double>(), " among bits with |llr| above 8", wrong.mul(confident).sum().item<double>() / confident.sum().item<double>());
}
```

Output

```text
llr shape [20000]
sign of the llr agrees with the hard decision for all but 0.0000 of the bits
error rate overall 0.0089  among bits with |llr| above 8 0.0001
```

The overall error rate is `0.89 %`, and among bits whose ratio is above `8` in size it is `0.01 %`.

## Against theory

`theory` gives the bit error probability of Gray-coded detection in white noise. The program simulates `100000` symbols of each of four constellations at one `Eb/N0` each and prints the measured rate next to the theory.

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
  struct Row { cm::Constellation c; const char *name; double ebn0; };
  const std::vector<Row> rows = {{cm::psk(2), "BPSK  ", 4.0}, {cm::psk(4), "QPSK  ", 6.0}, {cm::qam(16), "16-QAM", 8.0}, {cm::qam(64), "64-QAM", 12.0}};
  out.writeln("             Eb/N0   simulated    theory");
  std::uint64_t seed = 1;
  for (const Row &row : rows) {
    const pluss::Series bits = random_bits(row.c.bits * 100000, seed++);
    const pluss::Series received = cm::noise(cm::modulate(row.c, bits), cm::n0(row.ebn0, row.c), seed++);
    out.writeln(row.name, row.ebn0, cm::ber(bits, cm::demod(row.c, received)).item<double>(), cm::theory(row.c, vec({row.ebn0})).items<double>()[0]);
  }
}
```

Output

```text
             Eb/N0   simulated    theory
BPSK   4.000000 0.012140 0.012501
QPSK   6.000000 0.002415 0.002388
16-QAM 8.000000 0.009338 0.009247
64-QAM 12.000000 0.009778 0.009724
```

BPSK at `4 dB` measures `0.01214` against `0.01250`, QPSK at `6 dB` `0.002415` against `0.002388`, 16-QAM at `8 dB` `0.009338` against `0.009247` and 64-QAM at `12 dB` `0.009778` against `0.009724`. The theory is exact for BPSK and QPSK and a nearest-neighbour approximation for the rest; each simulated rate comes from a few thousand errors and is good to a few percent.

The whole waterfall is one call, because the inputs are arrays.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  // the whole waterfall in one call: a vector of Eb/N0 values gives a vector of probabilities
  const pluss::Series ebn0 = vec({0, 2, 4, 6, 8, 10, 12});
  out.writeln("Eb/N0 dB", flat(ebn0));
  out.writeln("BPSK  ", flat(cm::theory(cm::psk(2), ebn0)));
  out.writeln("16-QAM", flat(cm::theory(cm::qam(16), ebn0)));
  // a matrix of Eb/N0 values keeps its shape
  out.writeln("shape of a 2 x 2 input", cm::theory(cm::psk(4), vec({0, 4, 8, 12}).reshape({2, 2})).sizes());
}
```

Output

```text
Eb/N0 dB [0.00000000, 2.00000000, 4.00000000, 6.00000000, 8.00000000, 10.00000000, 12.00000000]
BPSK   [0.07864960, 0.03750613, 0.01250082, 0.00238829, 0.00019091, 0.00000387, 0.00000001]
16-QAM [0.13916001, 0.09755935, 0.05861846, 0.02787131, 0.00924721, 0.00175415, 0.00013866]
shape of a 2 x 2 input [2, 2]
```

## Quality of the symbols

The error vector magnitude is the root-mean-square distance of the received symbols from the ideal ones, relative to the ideal. For unit-energy symbols in noise of variance `N0` it is `sqrt(N0)`.

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

## Errors

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
  const cm::Constellation c = cm::psk(4);
  const pluss::Series received = cm::symbols(c, pluss::series::make(std::vector<std::int64_t>{0, 1}));
  report([&] { (void)cm::llr(c, received, 0.0); });
  report([&] { (void)cm::llr(c, received, -1.0); });
  report([&] { (void)cm::llr(c, pluss::Series{}, 0.5); });
}
```

Output

```text
eng::comms::llr: noise_variance must be positive
eng::comms::llr: noise_variance must be positive
eng::comms::llr: received must be a [..., N] array
```

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

## Decisions

- **How noisy is the channel?** State the `Eb/N0` and convert with `n0`; comparing different modulations at equal `Eb/N0` is fair, equal `N0` is not.
- **Hard bits or soft ratios?** `demod` for an uncoded link; `llr` when a decoder follows.
- **How many bits to simulate?** Enough to see at least a hundred errors; the relative uncertainty of a rate is about `1 / sqrt(errors)`.
- **A different noise realisation:** a different `seed`.

## Pitfalls

The program makes three mistakes: comparing two constellations at the same noise variance, trusting a bit error rate estimated from a handful of errors, and passing a noise variance to `llr` that does not match the channel.

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));

  // 1. the same N0 is a different Eb/N0 for different constellations
  const double n0 = 0.1;
  for (const cm::Constellation &c : {cm::psk(2), cm::psk(4), cm::qam(16)})
    out.writeln("order", c.order, "at N0 = 0.1, Eb/N0 in dB", 10.0 * std::log10(1.0 / (c.bits * n0)));

  // 2. a rate from a few errors is a guess: five repeats of 2000 bits at the same Eb/N0
  const cm::Constellation qpsk = cm::psk(4);
  std::vector<double> rates;
  for (std::uint64_t seed = 1; seed <= 5; ++seed) {
    const pluss::Series bits = random_bits(2 * 2000, seed);
    const pluss::Series received = cm::noise(cm::modulate(qpsk, bits), cm::n0(7.0, qpsk), 100 + seed);
    rates.push_back(cm::ber(bits, cm::demod(qpsk, received)).item<double>());
  }
  out.writeln("five measurements of the same rate", rates, " theory", cm::theory(qpsk, vec({7.0})).items<double>()[0]);

  // 3. confidence scaled for the wrong noise: the signs are right, the sizes are not
  const cm::Constellation bpsk = cm::psk(2);
  const pluss::Series received = cm::noise(cm::modulate(bpsk, random_bits(4, 2)), 0.5, 3);
  out.writeln("llr with the true variance 0.5 ", flat(cm::llr(bpsk, received, 0.5)));
  out.writeln("llr with a variance of 0.05    ", flat(cm::llr(bpsk, received, 0.05)));
}
```

Output

```text
order 2 at N0 = 0.1, Eb/N0 in dB 10.00000
order 4 at N0 = 0.1, Eb/N0 in dB 6.98970
order 16 at N0 = 0.1, Eb/N0 in dB 3.97940
five measurements of the same rate [0.00050, 0.00100, 0.00050, 0.00050, 0.00025]  theory 0.00077
llr with the true variance 0.5  [5.43579, 0.05838, -4.45008, 9.74924]
llr with a variance of 0.05     [54.35794, 0.58380, -44.50076, 97.49244]
```

At the same `N0 = 0.1` BPSK is at `10 dB`, QPSK at `6.99 dB` and 16-QAM at `3.98 dB`: compare at equal `Eb/N0`, not equal `N0`. Five measurements of the same QPSK rate from `4000` bits each give `0.0005` to `0.0010` against a theory of `0.00077`, with one to four errors each: far too few to trust a digit. And a variance ten times too small multiplies every ratio by ten (`5.4` becomes `54`): the signs are unchanged but a decoder would be overconfident.

## Where next

[Pulse shaping](pulse_shaping.md) puts the symbols on a band-limited waveform. Reference pages: [`noise`](../../../../api/eng/comms/channel/noise.md), [`noise_variance`](../../../../api/eng/comms/channel/n0.md), [`detect`](../../../../api/eng/comms/modulation/detect.md), [`demod`](../../../../api/eng/comms/modulation/demod.md), [`llr`](../../../../api/eng/comms/modulation/llr.md), [`theory`](../../../../api/eng/comms/metrics/theory.md), [`evm`](../../../../api/eng/comms/metrics/evm.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="constellations_and_mapping.html">
    <span class="label">Previous</span>
    <span class="title">Constellations and mapping</span>
  </a>
  <a class="next-link" href="pulse_shaping.html">
    <span class="label">Next</span>
    <span class="title">Pulse shaping</span>
  </a>
</div>
<!-- apiref:nav:end -->
