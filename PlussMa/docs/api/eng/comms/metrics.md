<!-- generated from metrics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Error metrics and theory

This branch measures a link. `ber` and `ser` count decisions, `evm` measures how far symbols are from their ideal positions, and `theory` gives the probability to compare against.

## Choosing A Surface

Count errors with `ber` and `ser`, measure the quality of symbols with `evm`, and compare with `theory`.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Bit errors.

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

EVM against noise.

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
### Example 3

Simulation against theory.

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
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

metrics/ber
metrics/ser
metrics/evm
metrics/theory
```
<!-- apiref:toctree:end -->
