# Energy levels and ground states

[Back to Physics 13: Energy Levels and Noise](../phys_13_levels_and_noise.md)

The energy levels of a quantum system, and the state of lowest energy, tell you most of what the system does at low temperature and how it behaves in time. You have a Hamiltonian for one spin, a pair or a chain, and you want its levels, its ground state, how that state changes as you tune a parameter, and how a mixture of two levels moves. This page diagonalises Hamiltonians with `quantum::spectrum` and `quantum::ground`, and follows states in time.

## How it works

A Hamiltonian {math}`H` is a Hermitian matrix. Its eigenvalues {math}`E_k` are the energy levels and its eigenvectors {math}`|k\rangle` the stationary states, which keep their form in time and only turn by a phase,

```{math}
H|k\rangle = E_k|k\rangle, \qquad |k\rangle \to e^{-iE_k t/\hbar}|k\rangle
```

`quantum::spectrum(H)` returns a `values` Series, in increasing order, and a `vectors` operator whose columns are the eigenvectors; `quantum::ground(H)` returns the lowest eigenvector as a ket. A system of several parts is built from operators of its parts with `quantum::tensor`, which multiplies the Hilbert spaces and records their sizes, so that operators and states of different systems cannot be mixed by mistake. The programs on this page build their Hamiltonians from the Pauli matrices {math}`X`, {math}`Y` and {math}`Z` and the identity, written as small helper functions at the top of each program, and combine them with `quantum::op(values, spaces)`. Expectation values, entropies and the evolution in time use the functions of Physics 3.

## Two coupled spins

The Heisenberg interaction {math}`H = X\otimes X + Y\otimes Y + Z\otimes Z` couples two spins. The program prints its levels, the ground state and the entropy of one spin of the ground state.

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
  // two spins coupled by H = X X + Y Y + Z Z: the Heisenberg interaction with strength 1
  const auto h = plus(plus(quantum::tensor(pauli_x(), pauli_x()), quantum::tensor(pauli_y(), pauli_y())), quantum::tensor(pauli_z(), pauli_z()));
  const auto levels = quantum::spectrum(h);
  out.writeln("energy levels = ", flat(levels.values));
  // the lowest level is the singlet; ground() returns it directly
  const auto lowest = quantum::ground(h);
  out.writeln("energy of the ground state = ", flat(quantum::expect(h, lowest)));
  out.writeln("amplitudes |00>, |01>, |10>, |11> (real part) = ", flat(lowest.value().real()), " up to a phase");
  out.writeln("probabilities                               = ", flat(quantum::prob(lowest)));
  // one spin of the singlet is a completely mixed state: its entropy is one bit
  const auto one_spin = quantum::partial_trace(quantum::density(lowest), {0});
  out.writeln("entropy of one spin = ", flat(quantum::entropy(one_spin)), " bit, purity = ", flat(quantum::purity(one_spin)));
  out.writeln("H is Hermitian: ", quantum::is_herm(h).item<bool>() ? "yes" : "no");
}
```

Output

```text
energy levels = [-3.0000, 1.0000, 1.0000, 1.0000]
energy of the ground state = [-3.0000]
amplitudes |00>, |01>, |10>, |11> (real part) = [0.0000, 0.7071, -0.7071, 0.0000] up to a phase
probabilities                               = [0.0000, 0.5000, 0.5000, 0.0000]
entropy of one spin = [1.0000] bit, purity = [0.5000]
H is Hermitian: yes
```

The four levels are `-3`, `1`, `1` and `1`: one level far below a degenerate triple. The ground state has energy `-3.0000` and equal probabilities `0.5000` for the states `|01>` and `|10>` and none for `|00>` and `|11>`, with amplitudes of opposite signs, the singlet. Its reduced state is completely mixed: one spin of the pair has purity `0.5000` and an entropy of `1.0000` bit, so the two spins are as entangled as two spins can be. The program also confirms `is_herm`.

## A chain of three spins in a field

A row of three spins with the interaction {math}`-Z\otimes Z` between neighbours and a field {math}`-h X` on each, {math}`H = -(Z_1 Z_2 + Z_2 Z_3) - h(X_1 + X_2 + X_3)`, is the simplest model of a magnet. The program finds the ground state for five field strengths and prints its energy, the gap to the first excited level and the mean of {math}`X` per spin.

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

// a chain of three spins in a transverse field: H = -(Z Z + Z Z) - h (X + X + X)
static quantum::Op chain(double field) {
  const auto I = identity2(), X = pauli_x(), Z = pauli_z();
  const auto zz01 = quantum::tensor(quantum::tensor(Z, Z), I), zz12 = quantum::tensor(quantum::tensor(I, Z), Z);
  const auto x0 = quantum::tensor(quantum::tensor(X, I), I), x1 = quantum::tensor(quantum::tensor(I, X), I), x2 = quantum::tensor(quantum::tensor(I, I), X);
  return plus(times(-1.0, plus(zz01, zz12)), times(-field, plus(plus(x0, x1), x2)));
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto x_total = plus(plus(quantum::tensor(quantum::tensor(pauli_x(), identity2()), identity2()), quantum::tensor(quantum::tensor(identity2(), pauli_x()), identity2())), quantum::tensor(quantum::tensor(identity2(), identity2()), pauli_x()));
  for (double field : {0.0, 0.5, 1.0, 2.0, 5.0}) {
    const auto h = chain(field);
    const auto levels = flat(quantum::spectrum(h).values);
    const auto g = quantum::ground(h);
    out.writeln("field ", field, ": ground energy ", levels[0], ", gap to the next level ", levels[1] - levels[0], ", mean X per spin ", flat(quantum::expect(x_total, g))[0] / 3.0);
  }
}
```

Output

```text
field 0.0000: ground energy -2.0000, gap to the next level 0.0000, mean X per spin 0.0000
field 0.5000: ground energy -2.4032, gap to the next level 0.1939, mean X per spin 0.5528
field 1.0000: ground energy -3.4940, gap to the next level 0.8901, mean X per spin 0.8424
field 2.0000: ground energy -6.2498, gap to the next level 2.7267, mean X per spin 0.9585
field 5.0000: ground energy -15.1000, gap to the next level 8.6384, mean X per spin 0.9933
```

Without a field the ground level is doubled, the spins all up or all down, so the gap is `0.0000` and the ground ket, which `ground` picks arbitrarily, has a mean of {math}`X` equal to `0.0000`. As the field grows the ground energy falls, `-2.4032`, `-3.4940`, `-6.2498`, `-15.1000`, the gap opens, `0.1939` to `8.6384`, and the spins turn along the field: the mean {math}`X` per spin goes from `0.5528` at `h = 0.5` to `0.9933` at `h = 5`. The state moves smoothly from a magnet, where the spins agree with each other, to a paramagnet, where each follows the field.

## A superposition of two levels

A state in a single level is stationary, and a state made of two levels beats at the frequency of their gap. The program takes the ground state and the first excited state of the chain with {math}`h = 1`, evolves each alone and then their equal superposition, and prints the overlap with the starting state at a quarter, a half and a whole revival time {math}`2\pi/\text{gap}`.

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
static quantum::Op chain(double field) {
  const auto I = identity2(), X = pauli_x(), Z = pauli_z();
  const auto zz01 = quantum::tensor(quantum::tensor(Z, Z), I), zz12 = quantum::tensor(quantum::tensor(I, Z), Z);
  const auto x0 = quantum::tensor(quantum::tensor(X, I), I), x1 = quantum::tensor(quantum::tensor(I, X), I), x2 = quantum::tensor(quantum::tensor(I, I), X);
  return plus(times(-1.0, plus(zz01, zz12)), times(-field, plus(plus(x0, x1), x2)));
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto h = chain(1.0);
  const auto levels = quantum::spectrum(h);
  const auto e = flat(levels.values);
  // the columns of the eigenvector matrix are the energy eigenstates; a state in one level only is stationary
  const auto first = quantum::ket(levels.vectors.value().select(-1, 0), h.spaces());
  const auto second = quantum::ket(levels.vectors.value().select(-1, 1), h.spaces());
  const auto times_ = vec({0.0, 1.0, 2.0, 5.0});
  out.writeln("overlap with the ground state at t = 0, 1, 2, 5, starting in it: ", flat(quantum::fidelity(first, quantum::schrod(h, first, times_))));
  // a superposition of the two lowest levels beats at their gap: it returns to itself after 2 pi / gap
  const double gap = e[1] - e[0], revival = 2.0 * std::numbers::pi / gap;
  const auto mixed = quantum::normalize(quantum::ket(first.value().add(second.value()), h.spaces()));
  const auto t = vec({0.0, revival / 4.0, revival / 2.0, revival});
  out.writeln("gap = ", gap, ", revival time 2 pi / gap = ", revival);
  out.writeln("t / revival time            = ", flat(t.div(revival)));
  out.writeln("overlap with the start      = ", flat(quantum::fidelity(mixed, quantum::schrod(h, mixed, t))));
  out.writeln("energy at every time        = ", flat(quantum::expect(h, quantum::schrod(h, mixed, t))), " (the mean of ", e[0], " and ", e[1], " is ", 0.5 * (e[0] + e[1]), ")");
}
```

Output

```text
overlap with the ground state at t = 0, 1, 2, 5, starting in it: [1.0000, 1.0000, 1.0000, 1.0000]
gap = 0.8901, revival time 2 pi / gap = 7.0591
t / revival time            = [0.0000, 0.2500, 0.5000, 1.0000]
overlap with the start      = [1.0000, 0.5000, 0.0000, 1.0000]
energy at every time        = [-3.0489, -3.0489, -3.0489, -3.0489] (the mean of -3.4940 and -2.6039 is -3.0489)
```

The ground state stays in itself: its overlap with the starting state is `1.0000` at every time. The gap is `0.8901`, so the revival time is `7.0591`. The superposition has an overlap of `0.5000` after a quarter of the revival time, `0.0000` after half, when it has become the orthogonal combination, and `1.0000` after the whole revival time. Its energy is constant at `-3.0489`, the mean of the two levels `-3.4940` and `-2.6039`, because the probabilities of the levels do not change.

## Decisions

- Use `ground` when you want only the lowest state, and `spectrum` when you need the levels or other eigenvectors. For a system of more than a few spins the full matrix grows as {math}`4^n`: the sparse operators of `quantum::sparse_op` are the way to reach larger sizes.
- Build a many-part operator from operators of one part with `tensor`, not by writing the large matrix by hand: it keeps the sizes of the parts, which `partial_trace` and the checks need.
- Use `schrod(H, ket, times)` for a time-dependent problem, and the levels when you want to understand the answer: a time evolution is a sum over levels, and the gaps are its frequencies.
- Remember that a degenerate level has no single ground state: `ground` returns one state of the level, and a quantity that distinguishes states within the level, such as a magnetisation, depends on that choice.

## Pitfalls

Operators that are not Hermitian, an observable and a state that belong to different systems, and a degenerate ground level. The program tries each.

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
  // a Hamiltonian with a complex off-diagonal term that is not matched by its conjugate is not Hermitian
  const auto skew = quantum::op(cx({{0, 0}, {1, 0}, {0, 0}, {0, 0}}, {2, 2}));
  out.writeln("is_herm: ", quantum::is_herm(skew).item<bool>() ? "yes" : "no");
  try { (void)quantum::spectrum(skew); } catch (const pluss::Error &e) { out.writeln("spectrum: ", e.message()); }
  try { (void)quantum::ground(skew); } catch (const pluss::Error &e) { out.writeln("ground: ", e.message()); }
  // an operator for one spin and a state of two spins: the sizes are given when the objects are built, and they must agree
  const auto pair = quantum::ket(cx({{1, 0}, {0, 0}, {0, 0}, {0, 0}}, {4}), {2, 2});
  try { (void)quantum::expect(pauli_z(), pair); } catch (const pluss::Error &e) { out.writeln("Z on a pair: ", e.message()); }
  // the operator built for the pair: Z on the first spin, identity on the second
  const auto z_first = quantum::tensor(pauli_z(), identity2());
  out.writeln("Z on the first spin of |00> = ", flat(quantum::expect(z_first, quantum::ket(cx({{1, 0}, {0, 0}, {0, 0}, {0, 0}}, {4}), {2, 2}))));
  // a degenerate ground level: any state in it is a ground state, and ground() returns just one of them
  const auto h = quantum::tensor(quantum::tensor(pauli_z(), pauli_z()), identity2());
  const auto g = quantum::ground(quantum::op(h.value().mul(-1.0), h.spaces()));
  out.writeln("levels of -Z Z (x) 1: ", flat(quantum::spectrum(quantum::op(h.value().mul(-1.0), h.spaces())).values));
  out.writeln("probabilities of the ground state returned: ", flat(quantum::prob(g)));
}
```

Output

```text
is_herm: no
spectrum: natsci::common::quantum::spectrum: expected a Hermitian operator
ground: natsci::common::quantum::spectrum: expected a Hermitian operator
Z on a pair: natsci::common::quantum::expect: observable and state must describe the same Hilbert space
Z on the first spin of |00> = [1.0000]
levels of -Z Z (x) 1: [-1.0000, -1.0000, -1.0000, -1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
probabilities of the ground state returned: [1.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000]
```

`is_herm` reports `no` for the operator with a single off-diagonal entry, and both `spectrum` and `ground` refuse it, because it has no real levels. The observable {math}`Z` for one spin cannot act on a pair, and the message says that the spaces differ; the operator `tensor(Z, 1)` that acts on the first spin gives `1.0000` for the state `|00>`. For {math}`-Z\otimes Z\otimes 1` the lowest level appears `4` times in a list of `8`, `-1.0000`, and `ground` returns one of the four, here `|000>`: do not read a physical meaning into which one.

## Where next

[Physics 3: Quantum Mechanics](../phys_03_quantum.md) follows a state in time and through the Lindblad equation, and [Quantum Channels](quantum_channels.md) describes the noise that moves a state away from the levels computed here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="quantum_channels.html">
    <span class="label">Next</span>
    <span class="title">Quantum channels and fidelity</span>
  </a>
</div>
<!-- apiref:nav:end -->
