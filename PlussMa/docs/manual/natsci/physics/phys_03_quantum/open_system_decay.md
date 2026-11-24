# Decay of an excited atom

[Back to Physics 3: Quantum Systems](../phys_03_quantum.md)

An atom in an excited level does not stay there: it emits light and falls to its lower level, and no single state vector describes the atom while it does. The atom is described by a density matrix and its decay by the Lindblad equation. This page lets an excited atom decay at a given rate and compares the population with the exponential law, and follows the purity, which falls while the atom is a mixture of levels and returns to one when it has settled in the lower level.

## How it works

`quantum::density(ket)` turns a state into a density matrix, and `quantum::lindblad(hamiltonian, rho, times, collapse)` evolves it under the Hamiltonian and under a list of collapse operators, one for each way the system loses energy to its surroundings. A collapse operator `sqrt(gamma) |0><1|` makes the atom fall from level `1` to level `0` at rate `gamma`. `quantum::expect(op, rho)` is the expected value of an observable, here the projector onto the upper level, which is the population of that level, and `quantum::purity(rho)` is `1` for a definite state and less for a mixture.

## Spontaneous emission

The atom starts in its upper level, has no driving field, and decays at rate `gamma = 0.5`. The program prints the population of the upper level and the exact `exp(-gamma t)` at five times, and the purity of the density matrix.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;

static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) {
  return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128);
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an excited two-level atom that emits light and falls to its lower level at rate gamma = 0.5; no driving field
  const auto upper = quantum::ket(cx({{0.0, 0.0}, {1.0, 0.0}}, {2}));
  const double gamma = 0.5;
  const auto fall = quantum::op(cx({{0.0, 0.0}, {std::sqrt(gamma), 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));   // sqrt(gamma) |0><1|
  const auto no_field = quantum::op(cx({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto rho = quantum::density(upper);

  const auto times = vec({0.0, 1.0, 2.0, 4.0, 8.0});
  const auto states = quantum::lindblad(no_field, rho, times, {fall});
  const auto in_upper = quantum::op(cx({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}}, {2, 2}));   // projector onto the upper level
  const auto population = quantum::expect(in_upper, states).real();
  out.writeln("time                          ", flat(times));
  out.writeln("probability of the upper level ", flat(population));
  std::vector<double> exact;
  for (double t : flat(times)) exact.push_back(std::exp(-gamma * t));
  out.writeln("exact exp(-gamma t)            ", exact);
  out.writeln("purity (1 = a definite state)  ", flat(quantum::purity(states)));
}
```

Output

```text
time                          [0.0000, 1.0000, 2.0000, 4.0000, 8.0000]
probability of the upper level [1.0000, 0.6065, 0.3679, 0.1353, 0.0183]
exact exp(-gamma t)            [1.0000, 0.6065, 0.3679, 0.1353, 0.0183]
purity (1 = a definite state)  [1.0000, 0.5227, 0.5349, 0.7660, 0.9640]
```

The population follows the exponential law exactly to the digits printed: `0.6065` at time `1`, `0.3679` at `2` and `0.0183` at `8`. The purity starts at `1.0000` for the definite excited state, falls to `0.5227` at time `1` because the atom is partly in each level, and rises again to `0.9640` by time `8` as the atom settles into its lower level, which is a definite state once more.

<!-- addex:start -->
## Dephasing

An atom can lose its coherence without losing energy. The collapse operator {math}`\sqrt{\gamma/2}\,\sigma_z` destroys the phase relation between the levels at rate {math}`\gamma`. The program starts the atom in the superposition {math}`(|0\rangle + |1\rangle)/\sqrt2`, which has {math}`\langle\sigma_x\rangle = 1`, and follows {math}`\langle\sigma_x\rangle` and the purity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>
#include <numbers>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;

static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) {
  return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128);
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // dephasing: a superposition (|0> + |1>) / sqrt 2 loses its coherence at rate gamma without losing any energy
  const double r = 1.0 / std::sqrt(2.0), gamma = 0.5;
  const auto plus = quantum::ket(cx({{r, 0.0}, {r, 0.0}}, {2}));
  const auto no_field = quantum::op(cx({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto sz = quantum::op(cx({{std::sqrt(gamma / 2.0), 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-std::sqrt(gamma / 2.0), 0.0}}, {2, 2}));   // sqrt(gamma / 2) sigma_z
  const auto sx = quantum::op(cx({{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto times = vec({0.0, 1.0, 2.0, 4.0, 8.0});
  const auto states = quantum::lindblad(no_field, quantum::density(plus), times, {sz});
  out.writeln("time                           ", flat(times));
  out.writeln("<sigma_x> (the coherence)      ", flat(quantum::expect(sx, states).real()));
  out.writeln("exact exp(-gamma t)            ", std::vector<double>{1.0, std::exp(-gamma), std::exp(-2 * gamma), std::exp(-4 * gamma), std::exp(-8 * gamma)});
  out.writeln("purity                         ", flat(quantum::purity(states)));
}
```

Output

```text
time                           [0.0000, 1.0000, 2.0000, 4.0000, 8.0000]
<sigma_x> (the coherence)      [1.0000, 0.6065, 0.3679, 0.1353, 0.0183]
exact exp(-gamma t)            [1.0000, 0.6065, 0.3679, 0.1353, 0.0183]
purity                         [1.0000, 0.6839, 0.5677, 0.5092, 0.5002]
```

The coherence decays exactly as {math}`e^{-\gamma t}` (`0.6065` at time `1`, `0.0183` at `8`), and the purity falls from `1.0000` to `0.5002`, the value of a completely mixed state, rather than returning to `1`: the populations of the two levels never changed, only the phase relation, so the atom ends up as an equal mixture.

## Driving against decay

When a field drives the atom while it decays, the two compete and the population settles to a steady value. The program drives an atom that starts excited with {math}`\omega = 2` while it decays at {math}`\gamma = 0.5` and prints the upper population up to time `32`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>
#include <numbers>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;

static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) {
  return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128);
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a resonantly driven atom that also decays (rate gamma = 0.5, drive omega = 2): the population settles to a steady value
  const double gamma = 0.5, omega = 2.0;
  const auto upper = quantum::ket(cx({{0.0, 0.0}, {1.0, 0.0}}, {2}));
  const auto h = quantum::op(cx({{0.0, 0.0}, {omega / 2.0, 0.0}, {omega / 2.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto fall = quantum::op(cx({{0.0, 0.0}, {std::sqrt(gamma), 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto in_upper = quantum::op(cx({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}}, {2, 2}));
  const auto times = vec({0.0, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0});
  const auto states = quantum::lindblad(h, quantum::density(upper), times, {fall});
  out.writeln("time                           ", flat(times));
  out.writeln("population of the upper level  ", flat(quantum::expect(in_upper, states).real()));
  out.writeln("steady state omega^2/4 / (omega^2/4*2 + gamma^2/4) = ", (omega * omega / 4.0) / (omega * omega / 2.0 + gamma * gamma / 4.0));
}
```

Output

```text
time                           [0.0000, 1.0000, 2.0000, 4.0000, 8.0000, 16.0000, 32.0000]
population of the upper level  [1.0000, 0.2425, 0.3789, 0.4359, 0.4619, 0.4859, 0.4848]
steady state omega^2/4 / (omega^2/4*2 + gamma^2/4) = 0.4848
```

The population falls quickly at first (`0.2425` at time `1`), then climbs back (`0.3789`, `0.4359`, `0.4619`) and settles at `0.4848`, the value {math}`\frac{\omega^2/4}{\omega^2/2 + \gamma^2/4}` of the steady state, which is just under one half: a strong drive can equalise the levels but cannot invert them.

<!-- addex:end -->

## Decisions

- Pass a jump operator `sqrt(rate) * L` for every loss channel; the rate is built into the operator.
- Use `no_field` (a zero Hamiltonian) for a pure decay and a real Hamiltonian for driving against it.
- Judge the result from the purity and from expectation values of operators such as the projector on the upper level, not from the state vector, which an open system no longer has.

## Pitfalls

The jump operator for a decay at rate `gamma` is `sqrt(gamma) |0><1|`. The program compares that with `|0><1|` on its own, for `gamma = 0.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;
static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) {
  return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128);
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the jump operator of decay at rate gamma is sqrt(gamma) |0><1|. Passing |0><1| itself means a rate of one, whatever gamma is
  const double gamma = 0.5;
  const auto upper = quantum::ket(cx({{0.0, 0.0}, {1.0, 0.0}}, {2}));
  const auto no_field = quantum::op(cx({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto in_upper = quantum::op(cx({{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}}, {2, 2}));
  const auto times = vec({0.0, 1.0, 2.0, 4.0});
  const auto rho = quantum::density(upper);
  const auto right = quantum::op(cx({{0.0, 0.0}, {std::sqrt(gamma), 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto wrong = quantum::op(cx({{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  out.writeln("time                          ", flat(times));
  out.writeln("sqrt(gamma) |0><1|            ", flat(quantum::expect(in_upper, quantum::lindblad(no_field, rho, times, {right})).real()));
  out.writeln("|0><1| (rate 1 by mistake)    ", flat(quantum::expect(in_upper, quantum::lindblad(no_field, rho, times, {wrong})).real()));
  out.writeln("exact exp(-gamma t)           ", std::vector<double>{1.0, std::exp(-gamma), std::exp(-2 * gamma), std::exp(-4 * gamma)});
}
```

Output

```text
time                          [0.0000, 1.0000, 2.0000, 4.0000]
sqrt(gamma) |0><1|            [1.0000, 0.6065, 0.3679, 0.1353]
|0><1| (rate 1 by mistake)    [1.0000, 0.3679, 0.1353, 0.0183]
exact exp(-gamma t)           [1.0000, 0.6065, 0.3679, 0.1353]
```

The correct operator gives `exp(-gamma t)`: `0.6065`, `0.3679` and `0.1353` at times 1, 2 and 4. Without the `sqrt(gamma)` the rate is `1` whatever you meant, and the population is already `0.3679` at time 1 and `0.0183` at time 4. The mistake runs without any error, so compare with a known limit as the page does.

## Where next

[A driven two-level atom](driven_two_level_atom.md) is the closed system from which this page starts. [Heat and work](../phys_04_thermodynamics/heat_and_work.md) covers energy flow into and out of a classical system.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="driven_two_level_atom.html">
    <span class="label">Previous</span>
    <span class="title">A driven two-level atom</span>
  </a>
</div>
<!-- apiref:nav:end -->
