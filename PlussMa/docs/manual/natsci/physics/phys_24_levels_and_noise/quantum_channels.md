# Quantum channels and fidelity

[Back to Physics 24: Energy Levels and Noise](../phys_24_levels_and_noise.md)

A quantum computer, a spin in a field, a photon in a fibre: any state that you prepare is disturbed before you read it, and you want to say by how much. A quantum channel is the map from the state you prepared to the state you have, and fidelity says how close the two are. This page builds channels from their Kraus operators, applies them to pure and mixed states, measures the damage with fidelity, purity and entropy, and follows repeated damping.

## How it works

A channel acts on a density matrix {math}`\rho` by a list of Kraus operators {math}`K_k`,

```{math}
\rho \;\longmapsto\; \sum_k K_k\,\rho\,K_k^{\dagger}, \qquad \sum_k K_k^{\dagger}K_k = 1
```

where the second condition says that the total probability stays one. `quantum::channel({K0, K1, ...})` builds a channel and checks that condition; `quantum::apply(channel, state)` applies it to a ket or to a density matrix and returns a density matrix. A pure state has purity {math}`\mathrm{tr}\,\rho^2 = 1` and a noisy one has less. `quantum::fidelity(a, b)` is {math}`|\langle a|b\rangle|^2` for two kets and {math}`\langle a|\rho|a\rangle` for a ket and a density matrix, and `quantum::root_fidelity` is its square root; both need normalised states, which `quantum::normalize` provides. `quantum::unitary(H, t)` is the operator {math}`e^{-iHt/\hbar}` that evolves a state under {math}`H` for a time {math}`t`, and it can be applied as a channel with the single Kraus operator {math}`U`. Entropy, `quantum::entropy(rho)`, is measured in bits by default.

## Two kinds of noise

A bit flip applies the Pauli matrix {math}`X` with probability {math}`p`, and a phase flip applies {math}`Z`. The program applies each to the state {math}`|0\rangle` and to {math}`|+\rangle = (|0\rangle + |1\rangle)/\sqrt2` for four probabilities and prints the population of {math}`|0\rangle`, the fidelity with the input and the purity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>
#include <numbers>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;
static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) { return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static quantum::Op pauli_x() { return quantum::op(cx({{0, 0}, {1, 0}, {1, 0}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_y() { return quantum::op(cx({{0, 0}, {0, -1}, {0, 1}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_z() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {-1, 0}}, {2, 2})); }
static quantum::Op identity2() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2})); }
static quantum::Op plus(const quantum::Op &a, const quantum::Op &b) { return quantum::op(a.value().add(b.value()), a.spaces()); }
static quantum::Op times(double c, const quantum::Op &a) { return quantum::op(a.value().mul(c), a.spaces()); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two noise channels, each given by its Kraus operators: with probability p the spin is flipped (X), or its phase is flipped (Z)
  const auto bit_flip = [](double p) { return quantum::channel({times(std::sqrt(1.0 - p), identity2()), times(std::sqrt(p), pauli_x())}); };
  const auto phase_flip = [](double p) { return quantum::channel({times(std::sqrt(1.0 - p), identity2()), times(std::sqrt(p), pauli_z())}); };
  const auto zero = quantum::ket(cx({{1, 0}, {0, 0}}, {2}));
  const auto plus_state = quantum::normalize(quantum::ket(cx({{1, 0}, {1, 0}}, {2})));
  const auto p0 = quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {0, 0}}, {2, 2}));
  for (double p : {0.0, 0.1, 0.3, 0.5}) {
    const auto a = quantum::apply(bit_flip(p), zero), b = quantum::apply(bit_flip(p), plus_state);
    const auto c = quantum::apply(phase_flip(p), zero), d = quantum::apply(phase_flip(p), plus_state);
    out.writeln("p = ", p);
    out.writeln("  bit flip,   |0>: population of |0> ", flat(quantum::expect(p0, a)), ", purity ", flat(quantum::purity(a)), "   |+>: fidelity ", flat(quantum::fidelity(plus_state, b)), ", purity ", flat(quantum::purity(b)));
    out.writeln("  phase flip, |0>: population of |0> ", flat(quantum::expect(p0, c)), ", purity ", flat(quantum::purity(c)), "   |+>: fidelity ", flat(quantum::fidelity(plus_state, d)), ", purity ", flat(quantum::purity(d)));
  }
}
```

Output

```text
p = 0.0000
  bit flip,   |0>: population of |0> [1.0000], purity [1.0000]   |+>: fidelity [1.0000], purity [1.0000]
  phase flip, |0>: population of |0> [1.0000], purity [1.0000]   |+>: fidelity [1.0000], purity [1.0000]
p = 0.1000
  bit flip,   |0>: population of |0> [0.9000], purity [0.8200]   |+>: fidelity [1.0000], purity [1.0000]
  phase flip, |0>: population of |0> [1.0000], purity [1.0000]   |+>: fidelity [0.9000], purity [0.8200]
p = 0.3000
  bit flip,   |0>: population of |0> [0.7000], purity [0.5800]   |+>: fidelity [1.0000], purity [1.0000]
  phase flip, |0>: population of |0> [1.0000], purity [1.0000]   |+>: fidelity [0.7000], purity [0.5800]
p = 0.5000
  bit flip,   |0>: population of |0> [0.5000], purity [0.5000]   |+>: fidelity [1.0000], purity [1.0000]
  phase flip, |0>: population of |0> [1.0000], purity [1.0000]   |+>: fidelity [0.5000], purity [0.5000]
```

The two channels hurt different states. A bit flip with {math}`p = 0.3` lowers the population of {math}`|0\rangle` to `0.7000` and the purity to `0.5800`, but leaves {math}`|+\rangle` untouched (fidelity `1.0000`, purity `1.0000`), because {math}`X|+\rangle = |+\rangle`. A phase flip does the opposite: {math}`|0\rangle` is unchanged and {math}`|+\rangle` falls to a fidelity of `0.7000` and a purity of `0.5800`. At {math}`p = 0.5` the damaged state is a half-and-half mixture, with purity `0.5000`.

## Fidelity, mixed states and a unitary pulse

The program compares states with `fidelity` and `root_fidelity`, compares a pure state with the completely mixed state, builds the unitary operator of a pulse {math}`H = X/2` of duration {math}`\pi`, and applies it as a channel with one Kraus operator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>
#include <numbers>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;
static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) { return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static quantum::Op pauli_x() { return quantum::op(cx({{0, 0}, {1, 0}, {1, 0}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_y() { return quantum::op(cx({{0, 0}, {0, -1}, {0, 1}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_z() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {-1, 0}}, {2, 2})); }
static quantum::Op identity2() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2})); }
static quantum::Op plus(const quantum::Op &a, const quantum::Op &b) { return quantum::op(a.value().add(b.value()), a.spaces()); }
static quantum::Op times(double c, const quantum::Op &a) { return quantum::op(a.value().mul(c), a.spaces()); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto zero = quantum::ket(cx({{1, 0}, {0, 0}}, {2}));
  const auto one = quantum::ket(cx({{0, 0}, {1, 0}}, {2}));
  const auto plus_state = quantum::normalize(quantum::ket(cx({{1, 0}, {1, 0}}, {2})));
  // fidelity measures how alike two states are: 1 for equal states, 0 for orthogonal ones
  out.writeln("fidelity(|0>, |0>) = ", flat(quantum::fidelity(zero, zero)), ", (|0>, |1>) = ", flat(quantum::fidelity(zero, one)), ", (|0>, |+>) = ", flat(quantum::fidelity(zero, plus_state)));
  out.writeln("root fidelity (|0>, |+>) = ", flat(quantum::root_fidelity(zero, plus_state)), " = |<0|+>|");
  // for a mixed state, compare a pure state and a density matrix; a completely mixed spin is 1/2 alike to any pure state
  const auto mixed = quantum::op(cx({{0.5, 0}, {0, 0}, {0, 0}, {0.5, 0}}, {2, 2}));
  out.writeln("fidelity(|0>, maximally mixed) = ", flat(quantum::fidelity(zero, mixed)), ", purity = ", flat(quantum::purity(mixed)), ", entropy = ", flat(quantum::entropy(mixed)), " bit");
  // U = exp(-i H t) for H = X / 2 and t = pi is -i X: it takes |0> to |1> up to a phase
  const auto u = quantum::unitary(times(0.5, pauli_x()), std::numbers::pi);
  out.writeln("real part of U      = ", flat(u.value().real()));
  out.writeln("imaginary part of U = ", flat(u.value().imag()));
  // as a channel with this one Kraus operator, the unitary acts on any state and gives a density matrix
  const auto p1 = quantum::op(cx({{0, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2}));
  const auto turned = quantum::apply(quantum::channel({u}), zero);
  out.writeln("population of |1> after the pulse = ", flat(quantum::expect(p1, turned)), ", purity = ", flat(quantum::purity(turned)));
}
```

Output

```text
fidelity(|0>, |0>) = [1.0000], (|0>, |1>) = [0.0000], (|0>, |+>) = [0.5000]
root fidelity (|0>, |+>) = [0.7071] = |<0|+>|
fidelity(|0>, maximally mixed) = [0.5000], purity = [0.5000], entropy = [1.0000] bit
real part of U      = [0.0000, 0.0000, 0.0000, 0.0000]
imaginary part of U = [0.0000, -1.0000, -1.0000, 0.0000]
population of |1> after the pulse = [1.0000], purity = [1.0000]
```

Equal states have fidelity `1.0000`, orthogonal ones `0.0000`, and {math}`|0\rangle` against {math}`|+\rangle` gives `0.5000`, whose square root `0.7071` is the overlap. The completely mixed state has purity `0.5000` and an entropy of `1.0000` bit, and its fidelity with any pure state is `0.5000`. The unitary for the pulse is {math}`-iX`: the real part is zero and the imaginary part has `-1.0000` in the off-diagonal places. Applied to {math}`|0\rangle`, it moves all the population to {math}`|1\rangle` (`1.0000`) and the state stays pure (purity `1.0000`), since a unitary loses no information.

## Repeated damping

Amplitude damping lets an excited spin drop to its lower level with probability {math}`\gamma`. Its Kraus operators are {math}`K_0 = \mathrm{diag}(1, \sqrt{1-\gamma})` and {math}`K_1 = \sqrt\gamma\,|0\rangle\langle1|`. The program applies the channel six times to the excited state with {math}`\gamma = 0.2` and prints the population, the purity and the entropy after each step, then looks at a superposition.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>
#include <numbers>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;
static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) { return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static quantum::Op pauli_x() { return quantum::op(cx({{0, 0}, {1, 0}, {1, 0}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_y() { return quantum::op(cx({{0, 0}, {0, -1}, {0, 1}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_z() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {-1, 0}}, {2, 2})); }
static quantum::Op identity2() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2})); }
static quantum::Op plus(const quantum::Op &a, const quantum::Op &b) { return quantum::op(a.value().add(b.value()), a.spaces()); }
static quantum::Op times(double c, const quantum::Op &a) { return quantum::op(a.value().mul(c), a.spaces()); }
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // amplitude damping with strength g: an excited spin drops to the lower level with probability g; K0 keeps the state, K1 is the jump
  const double g = 0.2;
  const auto damping = quantum::channel({quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {std::sqrt(1.0 - g), 0}}, {2, 2})), quantum::op(cx({{0, 0}, {std::sqrt(g), 0}, {0, 0}, {0, 0}}, {2, 2}))});
  const auto upper = quantum::density(quantum::ket(cx({{0, 0}, {1, 0}}, {2})));
  const auto p1 = quantum::op(cx({{0, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2}));
  // applying the same channel again and again: the excited population falls as (1 - g)^n
  auto state = upper;
  for (int n = 0; n <= 6; ++n) {
    out.writeln("after ", n, " applications: population ", flat(quantum::expect(p1, state)), " (formula ", std::pow(1.0 - g, n), "), purity ", flat(quantum::purity(state)), ", entropy ", flat(quantum::entropy(state)), " bit");
    state = quantum::apply(damping, state);
  }
  // a superposition loses its coherence at the slower rate sqrt(1 - g) per step
  const auto plus_state = quantum::density(quantum::normalize(quantum::ket(cx({{1, 0}, {1, 0}}, {2}))));
  const auto after = quantum::apply(damping, plus_state);
  out.writeln("off-diagonal element of |+><+| before and after one application: ", flat(plus_state.value().real())[1], " and ", flat(after.value().real())[1], " (sqrt(1 - g) / 2 = ", std::sqrt(1.0 - g) / 2.0, ")");
}
```

Output

```text
after 0 applications: population [1.0000] (formula 1.0000), purity [1.0000], entropy [0.0000] bit
after 1 applications: population [0.8000] (formula 0.8000), purity [0.6800], entropy [0.7219] bit
after 2 applications: population [0.6400] (formula 0.6400), purity [0.5392], entropy [0.9427] bit
after 3 applications: population [0.5120] (formula 0.5120), purity [0.5003], entropy [0.9996] bit
after 4 applications: population [0.4096] (formula 0.4096), purity [0.5163], entropy [0.9763] bit
after 5 applications: population [0.3277] (formula 0.3277), purity [0.5594], entropy [0.9125] bit
after 6 applications: population [0.2621] (formula 0.2621), purity [0.6132], entropy [0.8300] bit
off-diagonal element of |+><+| before and after one application: 0.5000 and 0.4472 (sqrt(1 - g) / 2 = 0.4472)
```

The excited population after {math}`n` applications is {math}`(1 - \gamma)^n` exactly: `0.8000`, `0.6400`, `0.5120`, ..., `0.2621` after six. The purity falls from `1.0000` to a minimum of `0.5003` after three applications, where the entropy is nearly one bit (`0.9996`), and rises again as the spin settles in the lower level (`0.6132` after six). A superposition loses its coherence more slowly than the population: the off-diagonal element of {math}`|+\rangle\langle+|` goes from `0.5000` to `0.4472`, which is {math}`\sqrt{1-\gamma}/2`.

## Decisions

- Describe noise by its Kraus operators and let `channel` check them: a mistake in the normalisation of the probabilities is caught when you build it, not after a long calculation.
- Use fidelity against the state you intended, and purity or entropy to see how much information was lost: a unitary changes the fidelity but not the purity, and noise changes both.
- Normalise a state before you compare it. Use `quantum::normalize` on anything that you built by adding amplitudes.
- Prefer a channel for noise that acts at one moment, as a measurement error does, and `lindblad` of Physics 3 for noise that acts continuously in time alongside a Hamiltonian.

## Pitfalls

Four mistakes that the library refuses: probabilities written where amplitudes belong, a state that is not normalised, a spin compared with a pair of spins, and a one-spin channel applied to a pair.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>
#include <numbers>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::physics::quantum;
using Complex = std::complex<double>;
static pluss::Series cx(const std::vector<Complex> &values, const std::vector<std::int64_t> &shape) { return pluss::make_series(values, shape).to(pluss::ScalarType::Complex128); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static quantum::Op pauli_x() { return quantum::op(cx({{0, 0}, {1, 0}, {1, 0}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_y() { return quantum::op(cx({{0, 0}, {0, -1}, {0, 1}, {0, 0}}, {2, 2})); }
static quantum::Op pauli_z() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {-1, 0}}, {2, 2})); }
static quantum::Op identity2() { return quantum::op(cx({{1, 0}, {0, 0}, {0, 0}, {1, 0}}, {2, 2})); }
static quantum::Op plus(const quantum::Op &a, const quantum::Op &b) { return quantum::op(a.value().add(b.value()), a.spaces()); }
static quantum::Op times(double c, const quantum::Op &a) { return quantum::op(a.value().mul(c), a.spaces()); }
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Kraus operators must satisfy sum K^dagger K = 1; a probability p entered as an amplitude breaks that
  try { (void)quantum::channel({times(0.9, identity2()), times(0.1, pauli_x())}); } catch (const pluss::Error &e) { out.writeln("amplitudes 0.9 and 0.1: ", e.message()); }
  // a Hamiltonian that is not Hermitian has no real energy levels
  const auto skew = quantum::op(cx({{0, 0}, {1, 0}, {0, 0}, {0, 0}}, {2, 2}));
  try { (void)quantum::spectrum(skew); } catch (const pluss::Error &e) { out.writeln("spectrum of a non-Hermitian operator: ", e.message()); }
  try { (void)quantum::ground(skew); } catch (const pluss::Error &e) { out.writeln("ground state of it: ", e.message()); }
  // an unnormalised state is not a state
  const auto big = quantum::ket(cx({{2, 0}, {0, 0}}, {2}));
  out.writeln("norm of the ket (2, 0) = ", flat(quantum::norm(big)));
  try { (void)quantum::fidelity(big, quantum::ket(cx({{1, 0}, {0, 0}}, {2}))); } catch (const pluss::Error &e) { out.writeln("fidelity of it with |0>: ", e.message()); }
  // a spin and a pair of spins are different Hilbert spaces: the sizes are given when the ket is built
  const auto pair = quantum::ket(cx({{1, 0}, {0, 0}, {0, 0}, {0, 0}}, {4}), {2, 2});
  const auto spin = quantum::ket(cx({{1, 0}, {0, 0}}, {2}), {2});
  try { (void)quantum::fidelity(spin, pair); } catch (const pluss::Error &e) { out.writeln("fidelity of a spin and a pair: ", e.message()); }
  // a channel written for one spin applied to a pair
  const auto flip = quantum::channel({times(std::sqrt(0.9), identity2()), times(std::sqrt(0.1), pauli_x())});
  try { (void)quantum::apply(flip, pair); } catch (const pluss::Error &e) { out.writeln("one-spin channel on a pair: ", e.message()); }
}
```

Output

```text
amplitudes 0.9 and 0.1: natsci::common::quantum::channel: Kraus operators are not trace preserving
spectrum of a non-Hermitian operator: natsci::common::quantum::spectrum: expected a Hermitian operator
ground state of it: natsci::common::quantum::spectrum: expected a Hermitian operator
norm of the ket (2, 0) = [2.0000]
fidelity of it with |0>: natsci::common::quantum::root_fidelity: states must be normalised (see normalize)
fidelity of a spin and a pair: natsci::common::quantum::inner: states must describe the same Hilbert space
one-spin channel on a pair: natsci::common::quantum::apply: channel and state must describe the same Hilbert space
```

Kraus operators of {math}`0.9` and {math}`0.1` instead of {math}`\sqrt{0.9}` and {math}`\sqrt{0.1}` are not trace preserving and the channel is refused. The ket `(2, 0)` has norm `2.0000`, and asking for its fidelity with {math}`|0\rangle` is refused with a message that points to `normalize`: an unnormalised state would otherwise give a number that is not a probability. A spin and a pair are different Hilbert spaces, and so are a one-spin channel and a pair, and both are refused. Give the sizes when you build a ket of several parts, `ket(amplitudes, {2, 2})`.

## Where next

[Energy levels and ground states](energy_levels.md) shows the states that noise acts on, and [Physics 3: Quantum Mechanics](../phys_03_quantum.md) models noise that acts continuously with the Lindblad equation, where the same decay appears as an exponential in time.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
