# Restricted Hartree-Fock

[Back to Chemistry 10: Hartree-Fock](../chem_10_hartree_fock.md)

Hartree-Fock is the first approximation of quantum chemistry: each electron moves in the average field of all the others, and the best set of orbitals is found by iterating until the field and the orbitals agree. This page runs that calculation for a closed-shell molecule from the integrals that describe its basis functions, and shows what the result contains, how to read the iteration, and how to restart and tune it. It also solves many geometries as the lanes of one call and differentiates the energy through the iteration, so that its derivatives with respect to the integrals come out of the same program.

## How it works


You supply the integrals over your basis functions; the calculation does not compute them from a geometry. With `n` basis functions (atomic orbitals), the integrals are the overlap matrix `S`, the core Hamiltonian `h` (kinetic energy plus attraction to the nuclei), the electron-repulsion integrals {math}`(\mu\nu|\lambda\sigma)` in chemists' notation, and the repulsion energy of the nuclei {math}`E_{nuc}`. The closed-shell energy is built from a density matrix `P` with {math}`P = 2 C_{occ} C_{occ}^T`, where the occupied orbitals {math}`C_{occ}` are the lowest `N/2` solutions of the Roothaan equations

```{math}
\begin{aligned}
&F C = S C\,\varepsilon, \\
&F_{\mu\nu} = h_{\mu\nu} + \sum_{\lambda\sigma} P_{\lambda\sigma}\left[(\mu\nu|\lambda\sigma) - \tfrac12(\mu\lambda|\nu\sigma)\right].
\end{aligned}
```

The Fock matrix `F` depends on `P`, so the equations are solved by iteration: guess `P`, build `F`, solve for new orbitals, form a new `P`, repeat. The electronic and total energies are

```{math}
\begin{aligned}
&E_{el} = \tfrac12 \sum_{\mu\nu} P_{\mu\nu}\,(h_{\mu\nu} + F_{\mu\nu}), \\
&E = E_{el} + E_{nuc}.
\end{aligned}
```

The iteration is accelerated by DIIS, which combines the last few Fock matrices to cancel their error {math}`FPS - SPF`; this error is also the orbital gradient that measures convergence. The first guess is the density built from the orbitals of the core Hamiltonian, which a restart replaces with your own density.

`pluss::ma::natsci::chemistry::quantum::rhf(integrals, electrons, options)` runs the calculation. The integrals are the four fields `overlap`, `core`, `eri` and `nuclear_energy`; `overlap` and `core` have shape `[..., n, n]` and `eri` has shape `[..., n, n, n, n]`, and any leading axes are batch lanes that are solved side by side. `rhf(integrals, electrons, initial_density, options)` starts from a density you supply. The options and the result fields are:

| Option | Default | Meaning |
|---|---|---|
| `max_iterations` | `64` | iteration budget |
| `energy_tolerance` | `1e-10` | stop when the energy changes by less than this |
| `gradient_tolerance` | `1e-8` | and the orbital gradient is below this |
| `damping` | `0` | fraction of the previous density mixed into each new one |
| `overlap_cutoff` | `1e-10` | smallest overlap eigenvalue treated as independent |
| `diis_start`, `diis_space`, `diis_rcond` | `1`, `8`, `1e-12` | first DIIS iteration, number of stored Fock matrices (`0` turns DIIS off), conditioning limit |
| `verify_symmetry` | `true` | refuse integrals that are not symmetric |

| Result field | Contents |
|---|---|
| `total_energy`, `electronic_energy` | {math}`E` and {math}`E_{el}` in hartree, one value per lane |
| `orbital_energy`, `coefficients` | the orbital energies {math}`\varepsilon` and the orbitals `C` (columns), lowest first |
| `density`, `fock` | the converged `P` and `F` |
| `density_rms`, `gradient_rms`, `energy_delta` | how much the density, the gradient and the energy changed on the last iteration |
| `converged`, `iterations`, `diis_steps` | whether both tolerances were met, and the iteration and DIIS counts | All results are Series, so a solution can be differentiated like any other expression.

## The hydrogen molecule

Two hydrogen atoms `1.4` bohr apart, each carrying one `1s` function, so that `n = 2`. The program supplies the standard integrals for this minimal basis, runs `rhf` for two electrons and prints the energies, the orbital energies and the two orbitals.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto result = quantum::rhf(h2_integrals(), 2);
  out.writeln("converged               ", result.converged.item<bool>() ? "yes" : "no", " after ", result.iterations, " iterations");
  out.writeln("total energy (hartree)  ", result.total_energy.item<double>());
  out.writeln("electronic energy       ", result.electronic_energy.item<double>());
  out.writeln("orbital energies        ", flat(result.orbital_energy));
  out.writeln("bonding orbital C[:,0]  ", flat(result.coefficients.select(1, 0)));
  out.writeln("antibonding C[:,1]      ", flat(result.coefficients.select(1, 1)));
}
```

Output

```text
converged               yes after 1 iterations
total energy (hartree)  -1.1168
electronic energy       -1.8310
orbital energies        [-0.5782, 0.6705]
bonding orbital C[:,0]  [-0.5489, -0.5489]
antibonding C[:,1]      [-1.2114, 1.2114]
```

The total energy is `-1.1168` hartree, the textbook value for this basis, and the bonding orbital has the lower energy `-0.5782` while the antibonding one sits at `+0.6705`. The bonding orbital has equal coefficients on the two atoms and the antibonding one has opposite signs, as symmetry requires (the overall sign of an orbital is arbitrary). The calculation converged in one iteration: for two identical atoms the core Hamiltonian already has the right symmetry, so its orbitals are the answer.

## A lopsided molecule and what to check

A model with two different basis functions, as in a diatomic of two different atoms: the core energies differ, so the iteration has real work to do. The program prints the convergence and the energies, then checks the density and the orbitals against the conditions they must satisfy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a lopsided two-orbital model, such as a heteronuclear diatomic: the two basis functions have different core energies, so the
// converged orbitals are not fixed by symmetry and the iteration has real work to do
static quantum::AoIntegrals lopsided() {
  return {pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
          pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
          pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01, 0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25}, {2, 2, 2, 2}),
          pluss::series::make(0.7)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8).with_sep(""));
  const auto ints = lopsided();
  quantum::HfOptions options;
  options.energy_tolerance = 1e-12;
  options.gradient_tolerance = 1e-11;
  const auto r = quantum::rhf(ints, 2, options);
  out.writeln("converged        ", r.converged.item<bool>() ? "yes" : "no", "   iterations = ", r.iterations, "   DIIS steps = ", r.diis_steps);
  out.writeln("total energy     ", r.total_energy.item<double>());
  out.writeln("orbital energies ", flat(r.orbital_energy));
  out.writeln("gradient rms     ", r.gradient_rms.item<double>());
  // the density matrix P = 2 C_occ C_occ^T counts the electrons through tr(P S) and satisfies P S P = 2 P
  const auto ps = r.density.matmul(ints.overlap);
  out.writeln("electrons tr(PS) ", ps.diagonal(0, -2, -1).sum().item<double>());
  out.writeln("|PSP - 2P| max   ", ps.matmul(r.density).sub(r.density.mul(2.0)).abs().max().item<double>());
  // the orbitals are orthonormal in the overlap metric: C^T S C = 1
  out.writeln("C^T S C          ", flat(r.coefficients.transpose(-2, -1).matmul(ints.overlap).matmul(r.coefficients)));
}
```

Output

```text
converged        yes   iterations = 10   DIIS steps = 9
total energy     -1.18034319
orbital energies [-0.68530923, 0.22110878]
gradient rms     0.00000000
electrons tr(PS) 2.00000000
|PSP - 2P| max   0.00000000
C^T S C          [1.00000000, 0.00000000, 0.00000000, 1.00000000]
```

It converges in `10` iterations, nine of them with DIIS, to `-1.18034319` hartree with a gradient of zero to eight places. The checks hold to the digits shown: the density counts `2.00000000` electrons through {math}`\mathrm{tr}(PS)`, it satisfies {math}`PSP = 2P` (the largest deviation prints as zero), and the orbitals are orthonormal in the overlap metric, {math}`C^T S C` equal to the identity.

## Options, restarts and a budget that is too small

The same lopsided model under five different regimes: the default, DIIS switched off, damping with DIIS off, a restart from the converged density, and an iteration budget of three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a lopsided two-orbital model, such as a heteronuclear diatomic: the two basis functions have different core energies, so the
// converged orbitals are not fixed by symmetry and the iteration has real work to do
static quantum::AoIntegrals lopsided() {
  return {pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
          pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
          pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01, 0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25}, {2, 2, 2, 2}),
          pluss::series::make(0.7)};
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8).with_sep(""));
  const auto ints = lopsided();
  const auto show = [&](const char *label, const quantum::RhfResult &r) {
    out.writeln(label, "  converged = ", r.converged.item<bool>() ? "yes" : "no", "  iterations = ", r.iterations, "  DIIS steps = ", r.diis_steps, "  E = ", r.total_energy.item<double>());
  };
  quantum::HfOptions base;
  base.energy_tolerance = 1e-12;
  base.gradient_tolerance = 1e-11;
  const auto reference = quantum::rhf(ints, 2, base);
  show("default (DIIS)       ", reference);
  // switch off the extrapolation: a plain fixed-point iteration reaches the same energy in more steps
  auto plain = base;
  plain.diis_space = 0;
  show("no DIIS              ", quantum::rhf(ints, 2, plain));
  // damping mixes a fraction of the previous density into each new one
  auto damped = plain;
  damped.damping = 0.5;
  damped.max_iterations = 200;
  show("damping 0.5, no DIIS ", quantum::rhf(ints, 2, damped));
  // restart from a converged density, as a continuation or a hand-made guess would
  show("restart from density ", quantum::rhf(ints, 2, reference.density, base));
  // an iteration budget that is too small reports the failure in `converged` instead of pretending
  auto short_budget = base;
  short_budget.max_iterations = 3;
  show("3 iterations allowed ", quantum::rhf(ints, 2, short_budget));
}
```

Output

```text
default (DIIS)         converged = yes  iterations = 10  DIIS steps = 9  E = -1.18034319
no DIIS                converged = yes  iterations = 19  DIIS steps = 0  E = -1.18034319
damping 0.5, no DIIS   converged = yes  iterations = 50  DIIS steps = 0  E = -1.18034319
restart from density   converged = yes  iterations = 1  DIIS steps = 0  E = -1.18034319
3 iterations allowed   converged = no  iterations = 3  DIIS steps = 2  E = -1.18034319
```

All regimes that converge reach the same energy, `-1.18034319`. DIIS cuts the work from `19` iterations to `10`; damping with a half-weighted old density takes `50`; a restart from the converged density needs only `1`. With `3` iterations allowed the program reports `converged = no` and the energy it has reached, which is already close, rather than pretending to have finished.

## Many geometries in one call

The lopsided two-orbital model of the previous sections can be solved for many geometries at once. Changing the nuclear repulsion changes the energy by a constant, and moving the nuclei changes the one-electron matrix. The program solves three nuclear energies on one lane axis, and three different one-electron matrices (the given one, and shifted up and down) on another. Each lane is a separate SCF problem.

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

- Leave DIIS on unless you are comparing iteration schemes; it is the difference between `10` and `19` iterations above. Use `damping` only when the iteration oscillates, and then together with a larger `max_iterations`.
- Tighten `energy_tolerance` and `gradient_tolerance` together. The energy error of a converged calculation is second order in the orbital error, so a loose gradient hides behind an apparently converged energy; derivatives and finite differences of the energy need the tight setting, a quick estimate the loose one.
- Pass an `initial_density` when you are following a molecule through a series of geometries, so that every calculation starts from the previous solution, or when a hand-made guess is better than the core Hamiltonian.
- Put many calculations in one call by giving the integrals leading batch axes (geometries, nuclear energies, parameter sets): each lane converges on its own and `converged` is reported per lane.
- Use `rhf` for closed-shell states and [`uhf`](unrestricted_open_shell.md) for open shells; for a closed shell they agree and `rhf` is cheaper.
- Pass `electrons` as the total number, which must be even and not more than twice the number of orbitals.

## Pitfalls

The inputs are checked before the iteration starts: the electron count has to be positive, even and fit in the basis; the matrices have to be symmetric; the overlap has to be positive definite above `overlap_cutoff`; and the repulsion tensor has to match the basis size.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>
using namespace pluss::tils;
namespace quantum = pluss::ma::natsci::chemistry::quantum;

// a lopsided two-orbital model, such as a heteronuclear diatomic: the two basis functions have different core energies, so the
// converged orbitals are not fixed by symmetry and the iteration has real work to do
static quantum::AoIntegrals lopsided() {
  return {pluss::series::make({{1.0, 0.2}, {0.2, 1.0}}),
          pluss::series::make({{-1.2, -0.25}, {-0.25, -0.4}}),
          pluss::series::make(std::vector<double>{0.53, 0.06, 0.06, 0.29, 0.06, 0.0125, 0.0125, 0.01, 0.06, 0.0125, 0.0125, 0.01, 0.29, 0.01, 0.01, 0.25}, {2, 2, 2, 2}),
          pluss::series::make(0.7)};
}

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try { call(); out.writeln(label, "  no error"); } catch (const pluss::Error &e) { out.writeln(label, "  ", e.message()); }
}

int main() {
  OutputEngine out;
  const auto ints = lopsided();
  // a closed-shell calculation needs an even number of electrons that fit in the basis
  attempt(out, "three electrons:   ", [&] { (void)quantum::rhf(ints, 3); });
  attempt(out, "six electrons:     ", [&] { (void)quantum::rhf(ints, 6); });
  // the matrices must be symmetric: a core Hamiltonian typed with one off-diagonal element wrong is refused
  auto skewed = ints;
  skewed.core = pluss::series::make({{-1.2, -0.25}, {-0.2, -0.4}});
  attempt(out, "asymmetric core:   ", [&] { (void)quantum::rhf(skewed, 2); });
  // two basis functions that are the same function make the overlap singular
  auto duplicate = ints;
  duplicate.overlap = pluss::series::make({{1.0, 1.0}, {1.0, 1.0}});
  attempt(out, "duplicate orbitals:", [&] { (void)quantum::rhf(duplicate, 2); });
  // the repulsion tensor has to match the basis size
  auto wrong = ints;
  wrong.eri = pluss::series::make(std::vector<double>(8, 0.1), {2, 2, 2});
  attempt(out, "eri of rank three: ", [&] { (void)quantum::rhf(wrong, 2); });
}
```

Output

```text
three electrons:       natsci::chemistry::quantum::hf: RHF requires a positive even electron count
six electrons:         natsci::chemistry::quantum::hf: RHF electron count cannot exceed 2*nao
asymmetric core:       natsci::chemistry::quantum::hf: overlap and core must be symmetric
duplicate orbitals:    natsci::chemistry::quantum::hf: overlap must be positive definite above overlap_cutoff
eri of rank three:     natsci::chemistry::quantum::hf: overlap/core need two matrix axes and eri needs four AO axes
```

Each mistake is refused with a message: three electrons, six electrons in a two-function basis, an asymmetric core Hamiltonian, two identical basis functions (a singular overlap) and a repulsion tensor of the wrong rank.

A calculation that runs out of iterations is not an error. The budget example above returns `converged = no` with an energy that already agrees with the converged one to six decimals, which is easy to miss. Test `converged`, which judges the energy change and the gradient against the tolerances, not the energy alone.

## Where next

[Unrestricted Hartree-Fock](unrestricted_open_shell.md) removes the requirement that each orbital be doubly occupied, so that radicals, triplets and stretched bonds can be treated, and uses the same options, results and batching. The results are Series, so reverse differentiation applies to any scalar built from them, not only to the energy. The [quantum mechanics chapter of the physics guide](../../physics/phys_03_quantum.md) describes the states and operators behind these orbitals.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="unrestricted_open_shell.html">
    <span class="label">Next</span>
    <span class="title">Unrestricted Hartree-Fock</span>
  </a>
</div>
<!-- apiref:nav:end -->
