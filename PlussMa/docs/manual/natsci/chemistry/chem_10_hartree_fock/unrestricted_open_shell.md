# Unrestricted Hartree-Fock

[Back to Chemistry 10: Hartree-Fock](../chem_10_hartree_fock.md)

Restricted Hartree-Fock puts the spin-up and spin-down electrons into the same orbitals, which only works when every electron has a partner. A radical, a molecule in a triplet state or a stretched bond has unpaired electrons. Unrestricted Hartree-Fock gives each spin its own orbitals. This page computes the singlet, the triplet and the one-electron cation of the hydrogen molecule, verifies two of them from first principles, and differentiates the energy with respect to the integrals. It then compares the triplet-singlet gap across a batch of geometries.

## How it works

The spin-up (alpha) and spin-down (beta) electrons have their own density matrices {math}`P^\alpha` and {math}`P^\beta` and Fock matrices {math}`F^\alpha, F^\beta`, which share the Coulomb field of the total density {math}`P = P^\alpha + P^\beta` and differ in the exchange, which only acts between electrons of the same spin:

```{math}
\begin{aligned}
&F^\alpha_{\mu\nu} = h_{\mu\nu} + \sum_{\lambda\sigma}\left[P_{\lambda\sigma}(\mu\nu|\lambda\sigma) - P^\alpha_{\lambda\sigma}(\mu\lambda|\nu\sigma)\right], \\
&E = \tfrac12\sum_{\mu\nu}\left[P_{\mu\nu}h_{\mu\nu} + P^\alpha_{\mu\nu}F^\alpha_{\mu\nu} + P^\beta_{\mu\nu}F^\beta_{\mu\nu}\right] + E_{nuc}.
\end{aligned}
```

The result is not forced to be a pure spin state, so the program reports the expectation value of the total spin squared, {math}`\langle S^2\rangle = S_z(S_z+1) + N_\beta - \sum_{ij}|\langle i^\alpha|j^\beta\rangle|^2`: it is `0` for a singlet, `3/4` for a doublet and `2` for a triplet, and a value above these says the solution is contaminated with higher spin states.

`pluss::ma::natsci::chemistry::quantum::uhf(integrals, alpha_electrons, beta_electrons, options)` takes the same integrals and options as `rhf` but counts the electrons per spin. `uhf(integrals, alpha_electrons, beta_electrons, guess, options)` starts from the pair of densities in `guess`, whose two fields are `alpha` and `beta`. The result carries the alpha and beta versions of the orbital energies, coefficients, density and Fock matrix, the same convergence fields as `rhf`, and `spin_squared`.

## Singlet, triplet and cation

The hydrogen molecule at `1.4` bohr, with the integrals of the previous page. The program runs three electron configurations, one alpha and one beta electron, two alpha electrons, and one alpha electron, and prints the energy and spin of each next to the restricted energy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// H2 in a minimal basis at a bond length of 1.4 bohr: the two 1s functions on the two nuclei.
// overlap S, core Hamiltonian h = T + V, and the repulsion integrals (mu nu | lambda sigma) with axes (mu, nu, lambda, sigma).
static quantum::AoIntegrals h2_integrals() {
  const double j11 = 0.7746, j12 = 0.5697, x = 0.4441, k12 = 0.2970;
  const std::vector<double> eri = {
      // (11|11) (11|12) (11|21) (11|22)     (12|11) (12|12) (12|21) (12|22)
      j11, x, x, j12, x, k12, k12, x,
      // (21|11) (21|12) (21|21) (21|22)     (22|11) (22|12) (22|21) (22|22)
      x, k12, k12, x, j12, x, x, j11};
  return {pluss::series::make({{1.0, 0.6593}, {0.6593, 1.0}}),
          pluss::series::make({{-1.1204, -0.9584}, {-0.9584, -1.1204}}),
          pluss::series::make(eri, {2, 2, 2, 2}),
          pluss::series::make(1.0 / 1.4)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto ints = h2_integrals();
  const auto show = [&](const char *label, const quantum::UhfResult &r) {
    out.writeln(label, " E = ", r.total_energy.item<double>(), "   <S^2> = ", r.spin_squared.item<double>(), "   converged = ", r.converged.item<bool>() ? "yes" : "no");
  };
  // the closed-shell molecule: one alpha and one beta electron reproduce the restricted result and a singlet <S^2> = 0
  show("singlet  (1 alpha, 1 beta)", quantum::uhf(ints, 1, 1));
  // two electrons with the same spin: the triplet, which has to put one of them in the antibonding orbital
  show("triplet  (2 alpha, 0 beta)", quantum::uhf(ints, 2, 0));
  // one electron: H2+, a doublet with <S^2> = 3/4
  show("cation   (1 alpha, 0 beta)", quantum::uhf(ints, 1, 0));
  out.writeln("restricted reference E = ", quantum::rhf(ints, 2).total_energy.item<double>());
}
```

Output

```text
singlet  (1 alpha, 1 beta) E = -1.11675   <S^2> = 0.00000   converged = yes
triplet  (2 alpha, 0 beta) E = -0.53164   <S^2> = 2.00000   converged = yes
cation   (1 alpha, 0 beta) E = -0.53853   <S^2> = 0.75000   converged = yes
restricted reference E = -1.11675
```

One alpha and one beta electron reproduce the restricted energy `-1.11675` with {math}`\langle S^2\rangle = 0`. Two alpha electrons are forced into different orbitals, the bonding and the antibonding one, and give `-0.53164` hartree, `0.585` hartree above the singlet, with {math}`\langle S^2\rangle = 2`, the triplet value {math}`S(S+1)` with `S = 1`. A single electron gives `-0.53853` and the doublet value `0.75`.

## Checking the triplet and the cation from first principles

By symmetry the two orbitals of the molecule are the bonding and the antibonding combination of the two basis functions. The program builds them by hand, evaluates the one-electron, Coulomb and exchange integrals with an explicit sum over the repulsion tensor, and compares with `uhf`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// H2 in a minimal basis at a bond length of 1.4 bohr: the two 1s functions on the two nuclei.
// overlap S, core Hamiltonian h = T + V, and the repulsion integrals (mu nu | lambda sigma) with axes (mu, nu, lambda, sigma).
static quantum::AoIntegrals h2_integrals() {
  const double j11 = 0.7746, j12 = 0.5697, x = 0.4441, k12 = 0.2970;
  const std::vector<double> eri = {
      // (11|11) (11|12) (11|21) (11|22)     (12|11) (12|12) (12|21) (12|22)
      j11, x, x, j12, x, k12, k12, x,
      // (21|11) (21|12) (21|21) (21|22)     (22|11) (22|12) (22|21) (22|22)
      x, k12, k12, x, j12, x, x, j11};
  return {pluss::series::make({{1.0, 0.6593}, {0.6593, 1.0}}),
          pluss::series::make({{-1.1204, -0.9584}, {-0.9584, -1.1204}}),
          pluss::series::make(eri, {2, 2, 2, 2}),
          pluss::series::make(1.0 / 1.4)};
}

using pluss::Series;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto ints = h2_integrals();
  // by symmetry the orbitals are the bonding g = (1 + 2) / sqrt(2 (1 + S)) and the antibonding u = (1 - 2) / sqrt(2 (1 - S))
  const double s = 0.6593;
  const auto g = pluss::series::make({1.0, 1.0}).div(std::sqrt(2.0 * (1.0 + s)));
  const auto u = pluss::series::make({1.0, -1.0}).div(std::sqrt(2.0 * (1.0 - s)));
  const auto core = [&](const Series &a) { return pluss::series::einsum("i,ij,j->", {a, ints.core, a}).item<double>(); };
  const auto coulomb = [&](const Series &a, const Series &b) { return pluss::series::einsum("i,j,ijkl,k,l->", {a, a, ints.eri, b, b}).item<double>(); };
  const auto exchange = [&](const Series &a, const Series &b) { return pluss::series::einsum("i,j,ijkl,k,l->", {a, b, ints.eri, a, b}).item<double>(); };
  const double nuclear = ints.nuclear_energy.item<double>();
  // the triplet has one electron in each orbital, so it feels their Coulomb repulsion minus the exchange between equal spins
  out.writeln("triplet from the orbitals  E = ", nuclear + core(g) + core(u) + coulomb(g, u) - exchange(g, u));
  out.writeln("triplet from uhf           E = ", quantum::uhf(ints, 2, 0).total_energy.item<double>());
  // H2+ has one electron, so there is no repulsion at all: E = <g|h|g> + nuclear repulsion
  out.writeln("cation from the orbital    E = ", nuclear + core(g));
  out.writeln("cation from uhf            E = ", quantum::uhf(ints, 1, 0).total_energy.item<double>());
}
```

Output

```text
triplet from the orbitals  E = -0.53164
triplet from uhf           E = -0.53164
cation from the orbital    E = -0.53853
cation from uhf            E = -0.53853
```

The triplet energy {math}`E_{nuc} + h_{gg} + h_{uu} + J_{gu} - K_{gu}` equals the `uhf` result, `-0.53164`, to every digit shown, and the cation energy {math}`E_{nuc} + h_{gg}` equals the one-electron result `-0.53853`: with one electron there is no repulsion to approximate, so Hartree-Fock is exact there.

## Differentiating the energy with respect to the integrals

Asking the core Hamiltonian and the nuclear repulsion to record their influence turns the energy into a function that can be differentiated back to them. The program runs the singlet, reads the slopes and compares them with the density.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// H2 in a minimal basis at a bond length of 1.4 bohr: the two 1s functions on the two nuclei.
// overlap S, core Hamiltonian h = T + V, and the repulsion integrals (mu nu | lambda sigma) with axes (mu, nu, lambda, sigma).
static quantum::AoIntegrals h2_integrals() {
  const double j11 = 0.7746, j12 = 0.5697, x = 0.4441, k12 = 0.2970;
  const std::vector<double> eri = {
      // (11|11) (11|12) (11|21) (11|22)     (12|11) (12|12) (12|21) (12|22)
      j11, x, x, j12, x, k12, k12, x,
      // (21|11) (21|12) (21|21) (21|22)     (22|11) (22|12) (22|21) (22|22)
      x, k12, k12, x, j12, x, x, j11};
  return {pluss::series::make({{1.0, 0.6593}, {0.6593, 1.0}}),
          pluss::series::make({{-1.1204, -0.9584}, {-0.9584, -1.1204}}),
          pluss::series::make(eri, {2, 2, 2, 2}),
          pluss::series::make(1.0 / 1.4)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  auto ints = h2_integrals();
  // ask the integrals to record their influence on the energy, run the calculation, and read the slopes back
  ints.core.tracks_slope_(true);
  ints.nuclear_energy.tracks_slope_(true);
  const auto result = quantum::uhf(ints, 1, 1);
  result.total_energy.reverse();
  // Hellmann-Feynman: the slope of the energy with respect to each core-Hamiltonian element is the total density matrix
  out.writeln("dE / d(core)       ", flat(ints.core.sensitivity()));
  out.writeln("total density      ", flat(result.alpha_density.add(result.beta_density)));
  // the nuclear repulsion enters the energy with coefficient one
  out.writeln("dE / d(nuclear)    ", ints.nuclear_energy.sensitivity().item<double>());
}
```

Output

```text
dE / d(core)       [0.60266, 0.60266, 0.60266, 0.60266]
total density      [0.60266, 0.60266, 0.60266, 0.60266]
dE / d(nuclear)    1.00000
```

The slope of the energy with respect to the core Hamiltonian is the total density matrix, `0.60266` in every element, as the Hellmann-Feynman theorem says: the energy changes with `h` only through the explicit term, because the orbitals are stationary. The slope with respect to the nuclear repulsion energy is `1.00000`, its coefficient in the energy. These derivatives are the starting point for forces and response properties.

## The triplet-singlet gap across a batch

The singlet and the triplet of the same integrals differ in the spin of the electrons and nothing else, and the gap between them is a quantity worth computing on its own. The program prints the triplet-singlet difference, repeats it for three nuclear repulsions in one call, and then scales every electron-repulsion integral by `0.8` to see which part of the integrals the gap depends on.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// H2 in a minimal basis at a bond length of 1.4 bohr: the two 1s functions on the two nuclei.
// overlap S, core Hamiltonian h = T + V, and the repulsion integrals (mu nu | lambda sigma) with axes (mu, nu, lambda, sigma).
static quantum::AoIntegrals h2_integrals() {
  const double j11 = 0.7746, j12 = 0.5697, x = 0.4441, k12 = 0.2970;
  const std::vector<double> eri = {
      // (11|11) (11|12) (11|21) (11|22)     (12|11) (12|12) (12|21) (12|22)
      j11, x, x, j12, x, k12, k12, x,
      // (21|11) (21|12) (21|21) (21|22)     (22|11) (22|12) (22|21) (22|22)
      x, k12, k12, x, j12, x, x, j11};
  return {pluss::series::make({{1.0, 0.6593}, {0.6593, 1.0}}),
          pluss::series::make({{-1.1204, -0.9584}, {-0.9584, -1.1204}}),
          pluss::series::make(eri, {2, 2, 2, 2}),
          pluss::series::make(1.0 / 1.4)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the same integrals, with the two electrons paired (singlet) and parallel (triplet)
  const auto ints = h2_integrals();
  const auto singlet = quantum::uhf(ints, 1, 1);
  const auto triplet = quantum::uhf(ints, 2, 0);
  out.writeln("triplet - singlet (hartree) ", triplet.total_energy.item<double>() - singlet.total_energy.item<double>());
  // the nuclear repulsion is a constant in the energy: three values of it are three lanes of one call, and every lane has the same gap
  auto batch = ints;
  batch.nuclear_energy = pluss::series::make(std::vector<double>{0.5, 1.0 / 1.4, 1.2}, {3});
  const auto s3 = quantum::uhf(batch, 1, 1);
  const auto t3 = quantum::uhf(batch, 2, 0);
  out.writeln("nuclear energy      ", flat(batch.nuclear_energy));
  out.writeln("singlet energy      ", flat(s3.total_energy));
  out.writeln("triplet energy      ", flat(t3.total_energy));
  out.writeln("triplet - singlet   ", flat(t3.total_energy.sub(s3.total_energy)));
  // the electron-electron part does move the gap: scale every repulsion integral by 0.8
  auto weak = ints;
  weak.eri = ints.eri.mul(0.8);
  out.writeln("gap with 0.8 x eri  ", quantum::uhf(weak, 2, 0).total_energy.item<double>() - quantum::uhf(weak, 1, 1).total_energy.item<double>());
}
```

Output

```text
triplet - singlet (hartree) 0.58511
nuclear energy      [0.50000, 0.71429, 1.20000]
singlet energy      [-1.33104, -1.11675, -0.63104]
triplet energy      [-0.74593, -0.53164, -0.04593]
triplet - singlet   [0.58511, 0.58511, 0.58511]
gap with 0.8 x eri  0.62355
```

The triplet lies `0.58511` hartree above the singlet, the value found above, and the three lanes with nuclear repulsions `0.50000`, `0.71429` and `1.20000` all show the same gap, `0.58511`: the nuclear repulsion is a constant added to both energies, so it cancels in the difference even though each energy shifts by it (the singlet goes from `-1.33104` to `-0.63104`). The electron repulsion does not cancel: with every repulsion integral scaled by `0.8` the gap becomes `0.62355`, so the gap carries the information about the electrons.

## Decisions

- Use `rhf` for a closed-shell molecule: it does less work and cannot break the spin symmetry. Use `uhf` when the numbers of alpha and beta electrons differ, and when you want to see whether a closed-shell molecule prefers to break its spin symmetry, for example when you stretch a bond.
- Read `spin_squared` next to every open-shell energy. A value close to {math}`S(S+1)` means a trustworthy spin state; a value well above it means the energy is contaminated and too low.
- The default guess starts both spins from the same core-Hamiltonian orbitals. If you want to look for a solution that breaks the spin symmetry, give `uhf` a guess in which the alpha and beta densities differ.
- The options of `uhf` are the ones of `rhf`, and DIIS acts on both spins together.
- Compare energies of different spin states only after reading `spin_squared`, and take differences within one call when you can: a batch of geometries gives every lane its own singlet and triplet, and constants such as the nuclear repulsion cancel in the difference.

## Pitfalls

The electron counts are per spin, and a restart density has to be symmetric. `uhf(integrals, 2, 1)` is a three-electron anion, not the neutral molecule, and it runs without complaint; zero electrons, more electrons of one spin than the basis has orbitals, a DIIS space of one and a lopsided guess are each refused with a message.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

#include <functional>

// H2 in a minimal basis at a bond length of 1.4 bohr: the two 1s functions on the two nuclei.
// overlap S, core Hamiltonian h = T + V, and the repulsion integrals (mu nu | lambda sigma) with axes (mu, nu, lambda, sigma).
static quantum::AoIntegrals h2_integrals() {
  const double j11 = 0.7746, j12 = 0.5697, x = 0.4441, k12 = 0.2970;
  const std::vector<double> eri = {
      // (11|11) (11|12) (11|21) (11|22)     (12|11) (12|12) (12|21) (12|22)
      j11, x, x, j12, x, k12, k12, x,
      // (21|11) (21|12) (21|21) (21|22)     (22|11) (22|12) (22|21) (22|22)
      x, k12, k12, x, j12, x, x, j11};
  return {pluss::series::make({{1.0, 0.6593}, {0.6593, 1.0}}),
          pluss::series::make({{-1.1204, -0.9584}, {-0.9584, -1.1204}}),
          pluss::series::make(eri, {2, 2, 2, 2}),
          pluss::series::make(1.0 / 1.4)};
}

static void attempt(OutputEngine &out, const char *label, const std::function<quantum::UhfResult()> &call) {
  try { const auto r = call(); out.writeln(label, "  E = ", r.total_energy.item<double>(), "  <S^2> = ", r.spin_squared.item<double>()); }
  catch (const pluss::Error &e) { out.writeln(label, "  ", e.message()); }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto ints = h2_integrals();
  // the electron counts are per spin: the call is uhf(integrals, alpha, beta), not the total
  attempt(out, "uhf(ints, 2, 1):      ", [&] { return quantum::uhf(ints, 2, 1); });
  attempt(out, "uhf(ints, 0, 0):      ", [&] { return quantum::uhf(ints, 0, 0); });
  // more electrons of one spin than the basis has orbitals
  attempt(out, "uhf(ints, 3, 0):      ", [&] { return quantum::uhf(ints, 3, 0); });
  // a DIIS space of one cannot extrapolate: use 0 to switch DIIS off, or 2 and more
  quantum::HfOptions bad;
  bad.diis_space = 1;
  attempt(out, "diis_space = 1:       ", [&] { return quantum::uhf(ints, 1, 1, bad); });
  // a restart density has to be symmetric: this one is not
  const quantum::UhfGuess lopsided{pluss::series::make({{1.0, 0.5}, {0.0, 0.0}}), pluss::series::zeros({2, 2})};
  attempt(out, "asymmetric restart:   ", [&] { return quantum::uhf(ints, 1, 0, lopsided); });
  // a valid call for comparison: two electrons of the same spin is the triplet, not a doublet
  attempt(out, "uhf(ints, 2, 0):      ", [&] { return quantum::uhf(ints, 2, 0); });
}
```

Output

```text
uhf(ints, 2, 1):        E = -0.44626  <S^2> = 0.75000
uhf(ints, 0, 0):        natsci::chemistry::quantum::hf: UHF spin populations must be non-negative and contain an electron
uhf(ints, 3, 0):        natsci::chemistry::quantum::hf: each UHF spin population cannot exceed nao
diis_space = 1:         natsci::chemistry::quantum::hf: DIIS start/space/rcond options are invalid
asymmetric restart:     natsci::chemistry::quantum::hf: alpha initial density must be symmetric
uhf(ints, 2, 0):        E = -0.53164  <S^2> = 2.00000
```

`uhf(ints, 2, 1)` returns an energy of `-0.44626` and a doublet, a legal calculation of the wrong molecule. Zero electrons, three alpha electrons in two orbitals, a DIIS space of one and an asymmetric restart density are each refused with a message; `uhf(ints, 2, 0)` is the triplet of two alpha electrons, as in the first example. Write `uhf(ints, 1, 1)` when you mean the singlet.

## Where next

[Restricted Hartree-Fock](restricted_closed_shell.md) explains the integrals, the options, the restarts and the result fields that this page reuses, and the batched and differentiated calculations. The [quantum mechanics chapter of the physics guide](../../physics/phys_03_quantum.md) covers density matrices, entropy and the operators that act on such states.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="restricted_closed_shell.html">
    <span class="label">Previous</span>
    <span class="title">Restricted Hartree-Fock</span>
  </a>
</div>
<!-- apiref:nav:end -->
