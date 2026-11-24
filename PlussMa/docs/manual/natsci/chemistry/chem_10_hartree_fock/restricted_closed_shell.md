# Restricted Hartree-Fock

[Back to Chemistry 10: Hartree-Fock](../chem_10_hartree_fock.md)

You have the integrals of a molecule in some atomic-orbital basis, and you want its self-consistent-field energy, orbitals and density for a closed-shell state in which every orbital holds an electron of each spin. This page solves the Roothaan equations with DIIS acceleration, reads the energy, the orbital energies and the density matrix, solves many geometries as the lanes of one call, and differentiates the energy with respect to the integrals. Both pages use one two-orbital model molecule whose integrals are given as numbers, so that nothing depends on a basis-set library: the overlap of the orbitals {math}`S_{12} = 0.2`, a one-electron matrix with diagonal {math}`-1.2` and {math}`-0.4` and off-diagonal {math}`-0.25`, a two-electron tensor with the largest element {math}`(11|11) = 0.53`, and a nuclear repulsion of {math}`0.7` hartree (atomic units).

## How it works

In the restricted method {math}`N` electrons fill the {math}`N/2` lowest orbitals, each doubly. With the atomic orbitals {math}`\chi_\mu` the orbitals are {math}`\psi_i = \sum_\mu C_{\mu i}\chi_\mu`, the density matrix is {math}`P = 2C_\mathrm{occ}C_\mathrm{occ}^{\top}`, and the Fock matrix is built from the one-electron matrix {math}`h` and the two-electron integrals {math}`(\mu\nu|\lambda\sigma)`,

```{math}
F_{\mu\nu} = h_{\mu\nu} + \sum_{\lambda\sigma} P_{\lambda\sigma}\left[(\mu\nu|\lambda\sigma) - \tfrac12(\mu\lambda|\nu\sigma)\right],\qquad FC = SC\varepsilon
```

The equations are solved by iteration until the Fock matrix and the density agree, and the energy is {math}`E = \tfrac12\mathrm{tr}[P(h+F)] + E_\mathrm{nuc}`. `quantum::AoIntegrals` holds the four inputs, `overlap` {math}`S`, `core` {math}`h`, `eri` and `nuclear_energy`, with optional leading batch axes; `quantum::rhf(integrals, electrons, options)` returns `total_energy`, `electronic_energy`, `orbital_energy`, `coefficients`, `density`, `fock`, the convergence measures `density_rms`, `gradient_rms`, `energy_delta`, `converged` and the iteration counts. Convergence is accelerated by DIIS, a least-squares combination of previous Fock matrices; `HfOptions` sets the tolerances, the iteration limit, the damping and the DIIS subspace. All results are Series, so a solution can be differentiated like any other expression.

## The energy, orbitals and density of the model molecule

The program solves the two-orbital model with two electrons and prints the convergence, the total and the electronic energy, the orbital energies, the density matrix, and the number of electrons it holds.

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
  const auto r = qc::rhf(model(), 2);   // two electrons in the two-orbital basis
  out.writeln("converged ", r.converged.item<bool>() ? "yes" : "no", ", iterations ", std::to_string(r.iterations), ", DIIS steps ", std::to_string(r.diis_steps));
  out.writeln("total energy      ", flat(r.total_energy));
  out.writeln("electronic energy ", flat(r.electronic_energy));
  out.writeln("orbital energies  ", flat(r.orbital_energy));
  out.writeln("density matrix    ", flat(r.density));
  // the density counts the electrons: trace(P S) = 2
  out.writeln("electrons         ", flat(r.density.matmul(model().overlap).diagonal(0, -2, -1).sum(-1, false)));
  out.writeln("gradient rms      ", flat(r.gradient_rms));
}
```

Output

```text
converged yes, iterations 9, DIIS steps 8
total energy      [-1.180343]
electronic energy [-1.880343]
orbital energies  [-0.685309, 0.221109]
density matrix    [1.911183, 0.179769, 0.179769, 0.016909]
electrons         [2.000000]
gradient rms      [0.000000]
```

The iteration converges in `9` steps, `8` of them DIIS steps, to a total energy of `-1.180343` hartree, of which `-1.880343` is electronic and `0.7` nuclear. The two orbital energies, `-0.685309` and `0.221109`, are the occupied and the empty orbital. The density matrix puts `1.911183` electrons on the first orbital and `0.016909` on the second, with the off-diagonal part `0.179769` that comes from the overlap and the mixing; the trace of {math}`PS` is `2.000000`, the number of electrons.

## Many geometries in one call

Changing the nuclear repulsion changes the energy by a constant, and moving the nuclei changes the one-electron matrix. The program solves three nuclear energies on one lane axis, and three different one-electron matrices (the given one, and shifted up and down) on another. Each lane is a separate SCF problem.

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
  // the nuclear repulsion enters as a constant: three geometries share the orbitals but have different nuclear energies
  auto m = model();
  m.nuclear_energy = pluss::series::make(std::vector<double>{0.5, 0.7, 1.2}, {3});
  const auto r = qc::rhf(m, 2);
  out.writeln("nuclear energy ", flat(m.nuclear_energy));
  out.writeln("total energy   ", flat(r.total_energy));
  out.writeln("converged      ", flat(r.converged.to(pluss::ScalarType::Float64)));
  // three different one-electron matrices (the nuclei moved): the lanes are solved independently
  const auto h0 = model().core;
  const auto shift = pluss::series::make({{0.08, -0.03}, {-0.03, 0.04}});
  const auto cores = h0.unsqueeze(0).unite({h0.add(shift).unsqueeze(0), h0.sub(shift).unsqueeze(0)}, 0);
  auto m2 = model();
  m2.core = cores;
  const auto r2 = qc::rhf(m2, 2);
  out.writeln("energies for three cores ", flat(r2.total_energy));
  out.writeln("lowest orbital energies  ", flat(r2.orbital_energy.select(-1, 0)));
}
```

Output

```text
nuclear energy [0.500000, 0.700000, 1.200000]
total energy   [-1.380343, -1.180343, -0.680343]
converged      [1.000000, 1.000000, 1.000000]
energies for three cores [-1.180343, -1.044836, -1.330321]
lowest orbital energies  [-0.685309, -0.628367, -0.751688]
```

With the integrals fixed, the total energy is the same electronic energy plus the nuclear energy: `-1.380343`, `-1.180343` and `-0.680343` for nuclear energies `0.5`, `0.7` and `1.2`, all converged. Three one-electron matrices give three independent solutions, `-1.180343`, `-1.044836` and `-1.330321`, and three lowest orbital energies, `-0.685309`, `-0.628367` and `-0.751688`: the lanes do not interact, and each has its own DIIS history.

## Derivatives of the energy through the SCF

A one-orbital problem with two electrons has the closed form {math}`E = 2h + (pp|pp) + E_\mathrm{nuc}` when {math}`S = 1`. The program marks the four inputs as tracked, solves, and asks for the derivative of the energy with respect to each by reverse differentiation through the whole SCF iteration.

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
  // one orbital with two electrons: E = 2 h + (pp|pp) + E_nuc. Track derivatives through the whole SCF.
  auto overlap = pluss::series::make(std::vector<double>{1.0}, {1, 1});
  auto core = pluss::series::make(std::vector<double>{-1.25}, {1, 1});
  auto eri = pluss::series::make(std::vector<double>{0.7}, std::vector<std::int64_t>{1, 1, 1, 1});
  auto nuclear = pluss::series::make(0.5);
  overlap.tracks_slope_(true);
  core.tracks_slope_(true);
  eri.tracks_slope_(true);
  nuclear.tracks_slope_(true);
  const auto r = qc::rhf({overlap, core, eri, nuclear}, 2);
  out.writeln("total energy ", flat(r.total_energy));
  r.total_energy.reverse();
  out.writeln("dE/dh          ", flat(core.sensitivity()));
  out.writeln("dE/d(pp|pp)    ", flat(eri.sensitivity()));
  out.writeln("dE/dE_nuc      ", flat(nuclear.sensitivity()));
  out.writeln("dE/dS          ", flat(overlap.sensitivity()));
}
```

Output

```text
total energy [-1.300000]
dE/dh          [2.000000]
dE/d(pp|pp)    [1.000000]
dE/dE_nuc      [1.000000]
dE/dS          [1.100000]
```

The energy is `-1.300000`, which is {math}`2(-1.25) + 0.7 + 0.5`. The derivatives are `2.000000` with respect to {math}`h` (two electrons feel it), `1.000000` for the two-electron integral and for the nuclear repulsion, as the closed form says, and `1.100000` for the overlap, which enters through the normalisation of the orbital: {math}`-2h/S^2 - 2(pp|pp)/S^3 = 2.5 - 1.4`. The same call gives the force on a nucleus once the integrals are functions of its position.

## Decisions

- Use `rhf` for closed-shell states and `uhf` ([Unrestricted Hartree-Fock](unrestricted_open_shell.md)) for open shells; for a closed shell they agree and `rhf` is cheaper.
- Pass `electrons` as the total number, which must be even and not more than twice the number of orbitals.
- Tighten `energy_tolerance` and `gradient_tolerance` for derivatives and finite differences of the energy, and loosen them for a quick estimate. Keep the DIIS space between {math}`2` and about {math}`8`.
- Pass a previous `density` as the initial density when you move a geometry slightly, so the new SCF starts near the answer.
- Use leading batch axes for geometries or parameter sets: the batch of integrals broadcasts against each other, and each lane converges on its own.

## Pitfalls

The method has requirements that are checked, and a convergence that is not an error. The program asks for an odd number of electrons, gives it two identical orbitals, asks for too many electrons, and limits the iterations to two.

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
  // an odd number of electrons has no closed-shell description
  try { qc::rhf(model(), 1); } catch (const pluss::Error &e) { out.writeln("odd electrons   : ", e.message()); }
  // two identical orbitals: the overlap matrix is singular
  try {
    auto bad = model();
    bad.overlap = pluss::series::make({{1.0, 1.0}, {1.0, 1.0}});
    qc::rhf(bad, 2);
  } catch (const pluss::Error &e) { out.writeln("singular overlap: ", e.message()); }
  // more electrons than orbitals can hold
  try { qc::uhf(model(), 3, 0); } catch (const pluss::Error &e) { out.writeln("too many electrons: ", e.message()); }
  // an iteration limit: the call returns, flagged as not converged
  qc::HfOptions few;
  few.max_iterations = 2;
  const auto r = qc::rhf(model(), 2, few);
  out.writeln("2 iterations: converged ", r.converged.item<bool>() ? "yes" : "no", ", energy ", flat(r.total_energy), ", gradient rms ", flat(r.gradient_rms));
  const auto ok = qc::rhf(model(), 2);
  out.writeln("default     : converged ", ok.converged.item<bool>() ? "yes" : "no", ", energy ", flat(ok.total_energy), ", gradient rms ", flat(ok.gradient_rms));
}
```

Output

```text
odd electrons   : natsci::chemistry::quantum::hf: RHF requires a positive even electron count
singular overlap: natsci::chemistry::quantum::hf: overlap must be positive definite above overlap_cutoff
too many electrons: natsci::chemistry::quantum::hf: each UHF spin population cannot exceed nao
2 iterations: converged no, energy [-1.180343], gradient rms [0.000038]
default     : converged yes, energy [-1.180343], gradient rms [0.000000]
```

An odd electron count, a singular overlap matrix (two orbitals that are the same function) and too many electrons are each rejected with a message that says which. An iteration limit that is too small is not an error: the call returns `converged no` with a gradient of `0.000038`, and the energy, to six decimals, already agrees with the converged one, `-1.180343`, which makes it easy to miss. Test `converged`, which judges the energy change and the gradient against the tolerances, not the energy alone.

## Where next

[Unrestricted Hartree-Fock](unrestricted_open_shell.md) removes the requirement that each orbital be doubly occupied. The results are Series, so reverse differentiation applies to any scalar built from them, not only to the energy.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="unrestricted_open_shell.html">
    <span class="label">Next</span>
    <span class="title">Unrestricted Hartree-Fock</span>
  </a>
</div>
<!-- apiref:nav:end -->
