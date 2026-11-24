# Unrestricted Hartree-Fock

[Back to Chemistry 10: Hartree-Fock](../chem_10_hartree_fock.md)

Not every molecule has paired electrons: a radical has one unpaired, and a triplet has two with parallel spins. You want the energy and orbitals of such states, and a number that tells you whether the solution is a clean spin state. This page solves the unrestricted equations, in which the electrons of each spin have orbitals of their own, and reads the expectation value of {math}`S^2`. Both pages use one two-orbital model molecule whose integrals are given as numbers, so that nothing depends on a basis-set library: the overlap of the orbitals {math}`S_{12} = 0.2`, a one-electron matrix with diagonal {math}`-1.2` and {math}`-0.4` and off-diagonal {math}`-0.25`, a two-electron tensor with the largest element {math}`(11|11) = 0.53`, and a nuclear repulsion of {math}`0.7` hartree (atomic units).

## How it works

With {math}`N_\alpha` electrons of spin up and {math}`N_\beta` of spin down, each spin gets its own coefficients and density matrix, {math}`P^\alpha` and {math}`P^\beta`, and the Fock matrices are

```{math}
F^{\sigma}_{\mu\nu} = h_{\mu\nu} + \sum_{\lambda\sigma'} (P^\alpha+P^\beta)_{\lambda\sigma'}(\mu\nu|\lambda\sigma') - \sum_{\lambda\sigma'} P^{\sigma}_{\lambda\sigma'}(\mu\lambda|\nu\sigma')
```

so that an electron feels the full Coulomb repulsion of all the others and the exchange only of its own spin. `quantum::uhf(integrals, n_alpha, n_beta, options)` returns separate `alpha_orbital_energy`, `beta_orbital_energy`, coefficients, densities and Fock matrices, the `total_energy`, and `spin_squared`, the expectation value {math}`\langle S^2\rangle` of the unrestricted determinant. For a pure spin state {math}`\langle S^2\rangle = S(S+1)`, {math}`0` for a singlet, {math}`0.75` for one unpaired electron, {math}`2` for a triplet; a larger value measures contamination by higher spin states. The options and the batch behaviour are those of `rhf`, and `UhfGuess` carries an initial pair of densities.

## A closed shell reproduces the restricted result

With one electron of each spin the unrestricted solution must equal the restricted one. The program solves both, and prints the energies, {math}`\langle S^2\rangle`, the orbital energies of both spins, and the iteration count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace qc = pluss::ma::natsci::chemistry::quantum;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A model molecule with two atomic orbitals. S is the overlap of the orbitals, h the one-electron (kinetic + nuclear attraction)
// matrix, eri[p][q][r][s] the two-electron repulsion integrals (pq|rs), and e_nuc the repulsion of the nuclei.
static qc::AoIntegrals model() {
  return {
      pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
      pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
      pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01,
                                              0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25},
                          {2, 2, 2, 2}),
      pluss::series::make(0.7),
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // a closed shell through the unrestricted code agrees with the restricted one, and has no spin contamination
  const auto closed = qc::uhf(model(), 1, 1);
  const auto rest = qc::rhf(model(), 2);
  out.writeln("UHF energy ", flat(closed.total_energy), ", RHF energy ", flat(rest.total_energy));
  out.writeln("S^2        ", flat(closed.spin_squared));
  out.writeln("alpha and beta orbital energies ", flat(closed.alpha_orbital_energy), flat(closed.beta_orbital_energy));
  out.writeln("converged ", closed.converged.item<bool>() ? "yes" : "no", ", iterations ", std::to_string(closed.iterations));
}
```

Output

```text
UHF energy [-1.180343], RHF energy [-1.180343]
S^2        [0.000000]
alpha and beta orbital energies [-0.685309, 0.221109][-0.685309, 0.221109]
converged yes, iterations 9
```

The energies are both `-1.180343`, and {math}`\langle S^2\rangle = 0`, a pure singlet. The alpha and beta orbital energies are identical, `-0.685309` and `0.221109`, the same as the restricted orbital energies of the previous page. The SCF converges in `9` iterations. For a closed shell the unrestricted method has nothing to add, which is why the restricted one is preferred there.

## One unpaired electron

With one electron and no beta electrons the molecule is a radical. The program prints its energy, {math}`\langle S^2\rangle`, the alpha orbital energies and the two densities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace qc = pluss::ma::natsci::chemistry::quantum;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A model molecule with two atomic orbitals. S is the overlap of the orbitals, h the one-electron (kinetic + nuclear attraction)
// matrix, eri[p][q][r][s] the two-electron repulsion integrals (pq|rs), and e_nuc the repulsion of the nuclei.
static qc::AoIntegrals model() {
  return {
      pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
      pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
      pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01,
                                              0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25},
                          {2, 2, 2, 2}),
      pluss::series::make(0.7),
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // one electron: a radical. The beta population is zero.
  const auto radical = qc::uhf(model(), 1, 0);
  out.writeln("energy                 ", flat(radical.total_energy));
  out.writeln("S^2 (exact value 0.75) ", flat(radical.spin_squared));
  out.writeln("alpha orbital energies ", flat(radical.alpha_orbital_energy));
  out.writeln("alpha density          ", flat(radical.alpha_density));
  out.writeln("beta density           ", flat(radical.beta_density));
}
```

Output

```text
energy                 [-0.500124]
S^2 (exact value 0.75) [0.750000]
alpha orbital energies [-1.200124, -0.073313]
alpha density          [0.994884, 0.012403, 0.012403, 0.000155]
beta density           [-0.000000, 0.000000, -0.000000, -0.000000]
```

The energy is `-0.500124` hartree, which is the lowest orbital energy of the one-electron Hamiltonian, `-1.200124`, plus the nuclear repulsion {math}`0.7`, as it must be for one electron with nothing to interact with. {math}`\langle S^2\rangle = 0.750000`, exactly {math}`\tfrac12(\tfrac12+1)`. The alpha density holds one electron, almost all on the first orbital (`0.994884`), and the beta density is zero.

## Singlet and triplet from the same integrals

Two electrons can be paired, {math}`(N_\alpha, N_\beta) = (1, 1)`, or parallel, {math}`(2, 0)`. The program solves both, and the triplet-singlet energy difference, and repeats the comparison for three nuclear repulsions in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace qc = pluss::ma::natsci::chemistry::quantum;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A model molecule with two atomic orbitals. S is the overlap of the orbitals, h the one-electron (kinetic + nuclear attraction)
// matrix, eri[p][q][r][s] the two-electron repulsion integrals (pq|rs), and e_nuc the repulsion of the nuclei.
static qc::AoIntegrals model() {
  return {
      pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
      pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
      pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01,
                                              0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25},
                          {2, 2, 2, 2}),
      pluss::series::make(0.7),
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // the same integrals, with the two electrons paired (singlet) and parallel (triplet)
  const auto singlet = qc::uhf(model(), 1, 1);
  const auto triplet = qc::uhf(model(), 2, 0);
  out.writeln("singlet energy, S^2 ", flat(singlet.total_energy), flat(singlet.spin_squared));
  out.writeln("triplet energy, S^2 ", flat(triplet.total_energy), flat(triplet.spin_squared));
  out.writeln("triplet - singlet (hartree) ", std::vector<double>{flat(triplet.total_energy)[0] - flat(singlet.total_energy)[0]});
  // the same comparison for three nuclear energies at once
  auto m = model();
  m.nuclear_energy = pluss::series::make(std::vector<double>{0.5, 0.7, 1.2}, {3});
  const auto s3 = qc::uhf(m, 1, 1);
  const auto t3 = qc::uhf(m, 2, 0);
  out.writeln("gap for nuclear energies 0.5, 0.7, 1.2 ", flat(t3.total_energy.sub(s3.total_energy)));
}
```

Output

```text
singlet energy, S^2 [-1.180343][0.000000]
triplet energy, S^2 [-0.573437][2.000000]
triplet - singlet (hartree) [0.606906]
gap for nuclear energies 0.5, 0.7, 1.2 [0.606906, 0.606906, 0.606906]
```

The singlet is at `-1.180343` hartree with {math}`\langle S^2\rangle = 0`, the triplet at `-0.573437` with {math}`\langle S^2\rangle = 2`, the value of {math}`S(S+1)` for {math}`S = 1`. The triplet lies `0.606906` hartree above, about {math}`16.5` eV, for this model with strong orbital overlap, and the difference is `0.606906` for all three nuclear energies: the nuclear repulsion is a constant that cancels in a difference.

## Decisions

- Specify the state through the electron counts {math}`(N_\alpha, N_\beta)`: the spin multiplicity is {math}`N_\alpha - N_\beta + 1`.
- Use {math}`\langle S^2\rangle` to judge the result. A value much above {math}`S(S+1)` means the determinant is contaminated by higher spin states and the energy is lower than a pure state would give.
- Pass a `UhfGuess` of two densities to start from a known state or to break the symmetry between the spins on purpose.
- Use `rhf` for closed shells; `uhf` costs about twice as much.

## Pitfalls

The unrestricted code takes the counts of the two spins, not the number of electrons and a multiplicity. The program checks the errors and then shows what {math}`(2, 0)` is.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace qc = pluss::ma::natsci::chemistry::quantum;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// A model molecule with two atomic orbitals. S is the overlap of the orbitals, h the one-electron (kinetic + nuclear attraction)
// matrix, eri[p][q][r][s] the two-electron repulsion integrals (pq|rs), and e_nuc the repulsion of the nuclei.
static qc::AoIntegrals model() {
  return {
      pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
      pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
      pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01,
                                              0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25},
                          {2, 2, 2, 2}),
      pluss::series::make(0.7),
  };
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // no electrons, and more electrons of one spin than there are orbitals
  try { qc::uhf(model(), 0, 0); } catch (const pluss::Error &e) { out.writeln("no electrons      : ", e.message()); }
  try { qc::uhf(model(), 3, 0); } catch (const pluss::Error &e) { out.writeln("three alpha in two orbitals: ", e.message()); }
  // a bad convergence setting
  try {
    qc::HfOptions bad;
    bad.diis_space = 1;
    qc::rhf(model(), 2, bad);
  } catch (const pluss::Error &e) { out.writeln("DIIS space of one: ", e.message()); }
  // the quiet mistake: two electrons as (2, 0) are the alpha and beta counts, which is a triplet, not a closed shell
  const auto wrong = qc::uhf(model(), 2, 0);
  const auto right = qc::uhf(model(), 1, 1);
  out.writeln("uhf(2, 0): energy ", flat(wrong.total_energy), ", S^2 ", flat(wrong.spin_squared));
  out.writeln("uhf(1, 1): energy ", flat(right.total_energy), ", S^2 ", flat(right.spin_squared));
}
```

Output

```text
no electrons      : natsci::chemistry::quantum::hf: UHF spin populations must be non-negative and contain an electron
three alpha in two orbitals: natsci::chemistry::quantum::hf: each UHF spin population cannot exceed nao
DIIS space of one: natsci::chemistry::quantum::hf: DIIS start/space/rcond options are invalid
uhf(2, 0): energy [-0.573437], S^2 [2.000000]
uhf(1, 1): energy [-1.180343], S^2 [0.000000]
```

Zero electrons, more electrons of one spin than orbitals, and an invalid DIIS setting are each rejected with a message. Asking for `uhf(2, 0)` is not an error, and it is not a closed shell: it is two alpha electrons, a triplet with {math}`\langle S^2\rangle = 2` and an energy of `-0.573437` instead of `-1.180343`. Write `uhf(1, 1)` when you mean the singlet.

## Where next

[Restricted Hartree-Fock](restricted_closed_shell.md) is the closed-shell method with the same options and the same batching. The orbitals and densities come back as Series, so they can be used by the density-based tools of [Physics 3: Quantum](../../physics/phys_03_quantum.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="restricted_closed_shell.html">
    <span class="label">Previous</span>
    <span class="title">Restricted Hartree-Fock</span>
  </a>
</div>
<!-- apiref:nav:end -->
