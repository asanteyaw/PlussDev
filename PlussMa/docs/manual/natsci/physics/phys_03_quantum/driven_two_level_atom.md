# A driven two-level atom

[Back to Physics 3: Quantum Systems](../phys_03_quantum.md)

The simplest quantum system has two levels. A field that couples them makes the atom oscillate between the two, and the oscillation can be solved exactly, which makes it a good first check on a quantum solver. This page follows an atom that starts in its lower level, compares the probability of finding it in the upper level with the exact formula, and checks that the state keeps its norm.

## How it works

A state is a `quantum::Ket` of complex amplitudes, made with `quantum::ket(amplitudes)`, and an observable or Hamiltonian is a `quantum::Op`, a complex matrix made with `quantum::op(matrix)`. `quantum::schrod(hamiltonian, initial, times)` solves the Schroedinger equation for every requested time at once and returns the state at each time; `quantum::prob(state)` gives the probability of each basis level, and `quantum::norm(state)` the length of the state vector, which a correct evolution keeps at `1`. Here `H = (omega / 2) sigma_x` couples the two levels with strength `omega`, and the probability of the upper level is `sin^2(omega t / 2)`.

## Rabi oscillation

The program starts the atom in its lower level with `omega = 2`, evolves it to six times and prints the probability of the upper level next to the exact value, and the norm of the state.

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
  // a two-level atom, starting in its lower level |0>, driven by a field: H = (omega / 2) sigma_x with omega = 2
  const auto lower = quantum::ket(cx({{1.0, 0.0}, {0.0, 0.0}}, {2}));
  const auto sigma_x = quantum::op(cx({{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto sigma_z = quantum::op(cx({{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}}, {2, 2}));
  const double omega = 2.0;
  const auto hamiltonian = quantum::op(sigma_x.value().mul(omega / 2.0));

  const auto times = vec({0.0, 0.5, 1.0, 1.5, std::numbers::pi / 2.0, std::numbers::pi});
  const auto states = quantum::schrod(hamiltonian, lower, times);
  const auto upper_probability = quantum::prob(states).select(-1, 1);
  out.writeln("time                          ", flat(times));
  out.writeln("probability of the upper level ", flat(upper_probability));
  std::vector<double> exact;
  for (double t : flat(times)) exact.push_back(std::pow(std::sin(omega * t / 2.0), 2));
  out.writeln("exact sin^2(omega t / 2)       ", exact);
  out.writeln("norm of the state              ", flat(quantum::norm(states)));
}
```

Output

```text
time                          [0.0000, 0.5000, 1.0000, 1.5000, 1.5708, 3.1416]
probability of the upper level [0.0000, 0.2298, 0.7081, 0.9950, 1.0000, 0.0000]
exact sin^2(omega t / 2)       [0.0000, 0.2298, 0.7081, 0.9950, 1.0000, 0.0000]
norm of the state              [1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
```

The computed probability and the exact `sin^2(omega t / 2)` agree to the four digits printed at every time: `0.2298` at time `0.5`, `0.7081` at `1.0`, and `1.0000` at `pi / 2`, where the atom has been completely transferred to the upper level. At time `pi` it has returned to the lower level (`0.0000`). The norm stays at `1.0000` throughout.

<!-- addex:start -->
## Off resonance

If the drive frequency does not match the atom, {math}`H = (\delta/2)\sigma_z + (\omega/2)\sigma_x` with detuning {math}`\delta`, the atom is never completely excited: the peak probability is {math}`\omega^2/(\omega^2 + \delta^2)` and the oscillation is faster. The program evolves the atom for four detunings with {math}`\omega = 2` and prints the largest probability reached in a run to time `8`.

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
  // an off-resonance drive: H = (delta / 2) sigma_z + (omega / 2) sigma_x. The atom never gets fully excited: the peak probability is omega^2 / (omega^2 + delta^2)
  const auto lower = quantum::ket(cx({{1.0, 0.0}, {0.0, 0.0}}, {2}));
  const auto sx = cx({{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}, {2, 2});
  const auto sz = cx({{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}}, {2, 2});
  const double omega = 2.0;
  std::vector<double> t; for (int k = 0; k <= 400; ++k) t.push_back(0.02 * k);
  for (double delta : {0.0, 1.0, 2.0, 4.0}) {
    const auto h = quantum::op(sx.mul(omega / 2.0).add(sz.mul(delta / 2.0)));
    const auto p = flat(quantum::prob(quantum::schrod(h, lower, vec(t))).select(-1, 1));
    out.writeln("detuning ", delta, ": peak probability of the upper level = ", *std::max_element(p.begin(), p.end()), ", predicted ", omega * omega / (omega * omega + delta * delta));
  }
}
```

Output

```text
detuning 0.0000: peak probability of the upper level = 1.0000, predicted 1.0000
detuning 1.0000: peak probability of the upper level = 0.8000, predicted 0.8000
detuning 2.0000: peak probability of the upper level = 0.5000, predicted 0.5000
detuning 4.0000: peak probability of the upper level = 0.2000, predicted 0.2000
```

The peak probability is `1.0000` on resonance, `0.8000` at a detuning of `1`, `0.5000` at `2` and `0.2000` at `4`, each equal to the predicted {math}`\omega^2/(\omega^2 + \delta^2)` to the digits printed.

## Two atoms, entangled

A state of several atoms is described by giving the dimensions of its parts, `quantum::ket(amplitudes, {2, 2})`, and `quantum::partial_trace(rho, {0})` discards all but the first atom. For an entangled pair each atom alone is a mixture, although the pair is in a definite state. `purity` is `1` for a definite state and `entropy` counts the missing information in bits. The program takes the Bell state {math}`(|00\rangle + |11\rangle)/\sqrt2` and, for comparison, the product state {math}`|00\rangle`.

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
  // two atoms in the entangled state (|00> + |11>) / sqrt 2: each alone is a completely mixed state
  const double r = 1.0 / std::sqrt(2.0);
  // the second argument says the four amplitudes describe two two-level systems, so that one of them can be traced out
  const auto bell = quantum::ket(cx({{r, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {r, 0.0}}, {4}), {2, 2});
  const auto pair = quantum::density(bell);
  out.writeln("purity of the pair                = ", flat(quantum::purity(pair)));
  const auto first = quantum::partial_trace(pair, {0});
  out.writeln("purity of one atom                = ", flat(quantum::purity(first)));
  out.writeln("entropy of one atom (bits)        = ", flat(quantum::entropy(first)));
  // a product state has no entanglement
  const auto zero = quantum::ket(cx({{1.0, 0.0}, {0.0, 0.0}}, {2}));
  const auto product = quantum::density(quantum::tensor(zero, zero));
  out.writeln("entropy of one atom of |00>       = ", flat(quantum::entropy(quantum::partial_trace(product, {0}))));
  // the overlap between the entangled pair and the product state
  out.writeln("fidelity of the pair with |00>    = ", flat(quantum::fidelity(quantum::tensor(zero, zero), pair)));
}
```

Output

```text
purity of the pair                = [1.0000]
purity of one atom                = [0.5000]
entropy of one atom (bits)        = [1.0000]
entropy of one atom of |00>       = [0.0000]
fidelity of the pair with |00>    = [0.5000]
```

The pair is in a pure state (purity `1.0000`), but one atom of it has purity `0.5000` and an entropy of `1.0000` bit: it is completely mixed, which is the signature of entanglement. In the product state one atom has zero entropy, and the fidelity of the entangled pair with {math}`|00\rangle` is `0.5000`.

<!-- addex:end -->

## Decisions

- Use `schrod` for an isolated system you can describe by a Hamiltonian and a starting ket; use `lindblad` once the system loses energy or coherence to its surroundings.
- Say how the amplitudes split into subsystems (the `{2, 2}` argument of `ket`) as soon as you will need a partial trace.
- Read the norm and the purity of the output: they should stay `1` for a closed system.

## Pitfalls

A Hamiltonian has to be Hermitian. The program evolves the same atom with a symmetric and with an unsymmetric two-level Hamiltonian.

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
  // a Hamiltonian must equal its own conjugate transpose. Here the two off-diagonal entries differ (1 and 0.5), so the evolution does not conserve probability
  const auto lower = quantum::ket(cx({{1.0, 0.0}, {0.0, 0.0}}, {2}));
  const auto times = vec({0.0, 1.0, 2.0, 3.0});
  const auto good = quantum::op(cx({{0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}}, {2, 2}));
  const auto bad = quantum::op(cx({{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.0}, {0.0, 0.0}}, {2, 2}));
  out.writeln("norm, Hermitian Hamiltonian     ", flat(quantum::norm(quantum::schrod(good, lower, times))));
  try {
    out.writeln("norm, non-Hermitian Hamiltonian ", flat(quantum::norm(quantum::schrod(bad, lower, times))));
  } catch (const pluss::Error &e) {
    out.writeln("non-Hermitian Hamiltonian: ", e.message());
  }
}
```

Output

```text
norm, Hermitian Hamiltonian     [1.0000, 1.0000, 1.0000, 1.0000]
non-Hermitian Hamiltonian: natsci::common::quantum::schrod: expected a Hermitian operator
```

The Hermitian Hamiltonian keeps the norm at `1.0000` at every time. The unsymmetric one is refused with `expected a Hermitian operator`, which stops it from silently losing or gaining probability.

A ket of four amplitudes can be one four-level system or two two-level atoms. The program keeps subsystem 0 of the density with and without the sizes, and then asks for subsystem 1 of the single system.

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
  // a ket of four amplitudes is one four-level system unless you say it is two two-level atoms. partial_trace keeps the subsystems you name, so keeping subsystem 0 of a single system keeps all of it
  const double r = 1.0 / std::sqrt(2.0);
  const auto amplitudes = cx({{r, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {r, 0.0}}, {4});
  const auto one_system = quantum::density(quantum::ket(amplitudes));
  out.writeln("no subsystem sizes: purity of what is kept ", flat(quantum::purity(quantum::partial_trace(one_system, {0}))));
  const auto two_atoms = quantum::density(quantum::ket(amplitudes, {2, 2}));
  out.writeln("sizes {2, 2}: purity of atom 0 ", flat(quantum::purity(quantum::partial_trace(two_atoms, {0}))));
  try {
    quantum::partial_trace(one_system, {1});
  } catch (const pluss::Error &e) {
    out.writeln("asking for atom 1 of the single system: ", e.message());
  }
}
```

Output

```text
no subsystem sizes: purity of what is kept [1.0000]
sizes {2, 2}: purity of atom 0 [0.5000]
asking for atom 1 of the single system: natsci::common::quantum::partial_trace: kept subsystem index is out of range
```

Without `{2, 2}` the state is a single system, so keeping subsystem 0 keeps all of it and the purity is `1.0000`; asking for subsystem 1 is refused as out of range. With the sizes the reduced state of one atom has purity `0.5000`, the signature of entanglement. State the subsystem sizes when you build the ket.

## Where next

[Decay of an excited atom](open_system_decay.md) adds the surroundings with the Lindblad equation. [Hamilton's equations](../phys_05_mechanics/hamiltons_equations.md) is the classical counterpart of the Hamiltonian used here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="open_system_decay.html">
    <span class="label">Next</span>
    <span class="title">Decay of an excited atom</span>
  </a>
</div>
<!-- apiref:nav:end -->
