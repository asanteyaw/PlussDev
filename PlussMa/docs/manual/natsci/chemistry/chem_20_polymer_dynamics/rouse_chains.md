# Rouse chains

[Back to Chemistry 20: Polymer dynamics](../chem_20_polymer_dynamics.md)

A polymer in a melt or a dilute solution is never still: its segments jiggle, the chain as a whole diffuses, and a stretched chain relaxes back. The Rouse model describes all of this with the least machinery that works: `N` beads joined by springs, each dragged through a medium with a friction `zeta`. This page builds the model's modes, then computes the relaxation times of a chain, its diffusion coefficient, and the stress relaxation and viscosity of a melt of such chains, and shows how each one scales with the chain length.

## How it works

Bead `n` sits at position {math}`r_n`, is joined to its neighbours by springs of constant {math}`k = 3 k_B T / b^2` (`b` is the bond length, the root-mean-square length of one spring) and feels a friction `zeta` and random thermal kicks. The springs couple the beads through the connectivity matrix `A`, which has `1` on the two chain ends, `2` on the interior diagonal and `-1` next to it. Its eigenvalues are known in closed form,

```{math}
a_p = 4 \sin^2\!\left(\frac{p\pi}{2N}\right), \qquad p = 0, 1, \dots, N-1 ,
```

and each eigenvector is an independent mode of motion. The mode `p = 0` has `a_0 = 0`: it is the translation of the whole chain. The other `N - 1` modes relax exponentially with the times

```{math}
\tau_p = \frac{\zeta b^2}{3 k_B T\, a_p} \;\longrightarrow\; \frac{\zeta N^2 b^2}{3\pi^2 k_B T\, p^2} \quad (p \ll N),
```

so the slowest mode, which is the end-to-end relaxation of the whole chain, takes a time proportional to {math}`N^2`. The translation mode gives the diffusion coefficient {math}`D = k_B T / (N\zeta)`. For `c` chains per unit volume the stress relaxation modulus and the zero-shear viscosity follow from the same times:

```{math}
G(t) = c\, k_B T \sum_{p=1}^{N-1} e^{-2t/\tau_p}, \qquad \eta = \frac{c\, k_B T}{2}\sum_{p=1}^{N-1}\tau_p \;\longrightarrow\; \frac{c\,\zeta N^2 b^2}{36}.
```

The functions are in `pluss::ma::natsci::chemistry::polymer_dynamics`: `rouse_matrix(n_beads)` and `rouse_eigenvalues(n_beads)` give `A` and its spectrum `a_p`; `rouse_relaxation_times(n_beads, zeta, b, kt)` returns the `N - 1` times `tau_p`; `rouse_diffusion(n_beads, zeta, kt)` the diffusion coefficient; `rouse_modulus(t, tau, chains_per_volume, kt)` the modulus at each time in `t`; and `rouse_viscosity(tau, chains_per_volume, kt)` the viscosity. The friction `zeta` is a Series, so a set of frictions runs as a batch of lanes; the bond length, the thermal energy and the chain density may be plain numbers or Series of the same shape.

## The matrix and its modes

The program builds the connectivity matrix of an eight-bead chain, prints two of its rows, and compares the eigenvalues from the closed form with the spectrum of the matrix itself.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace pd = chemistry::polymer_dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // a chain of 8 beads: the connectivity matrix and its eigenvalues a_p = 4 sin^2(p pi / 16)
  out.writeln("connectivity row 0 ", flat(pd::rouse_matrix(8).select(0, 0)));
  out.writeln("connectivity row 3 ", flat(pd::rouse_matrix(8).select(0, 3)));
  const auto a = pd::rouse_eigenvalues(8);
  out.writeln("eigenvalues        ", flat(a));
  // the spectrum of the matrix itself agrees with the formula
  const auto numeric = pluss::ma::math::linalg::eigh(pd::rouse_matrix(8)).eigenvalues;
  out.writeln("numeric spectrum   ", flat(numeric));
}
```

Output

```text
connectivity row 0 [1.000000, -1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000]
connectivity row 3 [0.000000, 0.000000, -1.000000, 2.000000, -1.000000, 0.000000, 0.000000, 0.000000]
eigenvalues        [0.000000, 0.152241, 0.585786, 1.234633, 2.000000, 2.765367, 3.414214, 3.847759]
numeric spectrum   [-0.000000, 0.152241, 0.585786, 1.234633, 2.000000, 2.765367, 3.414214, 3.847759]
```

Row `0` carries `1` on the chain end and `-1` for its single neighbour; row `3` is an interior bead with `2` on the diagonal and `-1` on both sides. The first eigenvalue is zero, the translation of the whole chain (the numerical spectrum prints it as a rounding-sized `-0.000000`), and the rest rise from `0.152241` to `3.847759`; the closed form and the numerical spectrum agree to all six digits shown.

## Relaxation times, diffusion and viscosity of one chain

A chain of `50` beads with a bead friction of {math}`10^{-12}` kg/s, a bond length of `1` nm and the thermal energy at `298` K. The program prints the first four relaxation times next to the long-chain formula, then the diffusion coefficient and the viscosity of one chain per unit volume divided by `kT`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace pd = chemistry::polymer_dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  // a chain of N = 50 beads: friction 1e-12 kg/s per bead, bond length 1 nm, kT at 298 K
  const double kt = 4.114e-21, b = 1e-9;
  const std::int64_t n = 50;
  const auto zeta = vec({1e-12});
  const auto tau = pd::rouse_relaxation_times(n, zeta, b, kt);  // [1, N-1]
  out.writeln("tau_1 .. tau_4 (s)       ", flat(tau.slice(-1, 0, 4, 1)));
  // the long-chain formula tau_p = zeta N^2 b^2 / (3 pi^2 kT p^2) for the slowest modes
  std::vector<double> approx;
  for (int p = 1; p <= 4; ++p) approx.push_back(1e-12 * n * n * b * b / (3.0 * std::numbers::pi * std::numbers::pi * kt * p * p));
  out.writeln("long-chain formula (s)   ", approx);
  // diffusion of the whole chain: kT / (N zeta)
  out.writeln("D (m^2/s)                ", flat(pd::rouse_diffusion(n, zeta, kt)));
  // the zero-shear viscosity contribution of one chain per cubic metre
  out.writeln("eta / (c kT)  (s)        ", flat(pd::rouse_viscosity(tau, 1.0, kt).div(kt)));
}
```

Output

```text
tau_1 .. tau_4 (s)       [2.0530e-08, 5.1377e-09, 2.2872e-09, 1.2895e-09]
long-chain formula (s)   [2.0524e-08, 5.1309e-09, 2.2804e-09, 1.2827e-09]
D (m^2/s)                [8.2280e-11]
eta / (c kT)  (s)        [1.6873e-08]
```

The slowest mode relaxes in `20.5` ns and the next ones in `5.14`, `2.29` and `1.29` ns: they fall as `1/p^2`, and the long-chain formula reproduces them to better than `0.1%`. The chain diffuses at {math}`8.228\times10^{-11}` m²/s, exactly {math}`k_B T/(N\zeta)`. The viscosity per unit `c kT` is {math}`1.6873\times10^{-8}` s, which is the long-chain value {math}`\zeta N^2 b^2/(36 k_B T) = 1.6880\times10^{-8}` s times the exact factor {math}`(N^2-1)/N^2`.

## Stress relaxation and chain length

A melt of {math}`10^{24}` chains per cubic metre, each of `50` beads: the program prints the modulus at five times, and then, for chains of `20`, `40` and `80` beads, the products {math}`D N \zeta / k_B T` and {math}`\eta/(c\zeta b^2 N^2)`, which the theory says are constants.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace pd = chemistry::polymer_dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  const double kt = 4.114e-21, b = 1e-9, c = 1e24;  // chains per cubic metre
  const auto zeta = vec({1e-12});
  // stress relaxation of a melt of 50-bead chains: G(t) = c kT sum_p exp(-2 t / tau_p)
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto tau = pd::rouse_relaxation_times(50, zeta, b, kt);
  const auto t = vec({1e-10, 1e-9, 1e-8, 1e-7, 1e-6});
  const auto g = pd::rouse_modulus(t, tau, c, kt);  // [1, T]
  out.writeln("t (s)      ", flat(t));
  out.writeln("G(t) (Pa)  ", flat(g));
  // doubling the chain length: diffusion halves and the viscosity quadruples
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  for (const std::int64_t n : {20, 40, 80}) {
    const auto tn = pd::rouse_relaxation_times(n, zeta, b, kt);
    const double d = flat(pd::rouse_diffusion(n, zeta, kt))[0];
    const double eta = flat(pd::rouse_viscosity(tn, c, kt))[0];
    out.writeln("N = ", n, "   D N / (kT/zeta) = ", d * n * 1e-12 / kt, "   eta / (c zeta b^2 N^2) = ", eta / (c * 1e-12 * b * b * n * n));
  }
}
```

Output

```text
t (s)      [1.0000e-10, 1.0000e-09, 1.0000e-08, 1.0000e-07, 1.0000e-06]
G(t) (Pa)  [3.5954e+04, 9.6523e+03, 1.6376e+03, 2.4184e-01, 2.0269e-39]
N = 20   D N / (kT/zeta) = 1.0000   eta / (c zeta b^2 N^2) = 0.0277
N = 40   D N / (kT/zeta) = 1.0000   eta / (c zeta b^2 N^2) = 0.0278
N = 80   D N / (kT/zeta) = 1.0000   eta / (c zeta b^2 N^2) = 0.0278
```

The modulus starts near {math}`c k_B T (N-1) \approx 2\times10^5` Pa and falls through `3.6e4` Pa at `0.1` ns, `1.6e3` Pa at `10` ns and `0.24` Pa at `100` ns: every mode has to relax, and the slowest one, at `20.5` ns, sets the final decay. Across chain lengths the first product is `1.0000` for every length, so {math}`D \propto 1/N`, and the second settles at `0.0277`, `0.0278`, `0.0278`, which is {math}`1/36`: the viscosity grows as {math}`N^2`.

## Decisions

- Use Rouse when each bead moves in its own friction and does not feel the flow it creates around the others: a melt, where the surrounding chains screen the flow, or a chain so stiff and short that the flow around it is irrelevant. For a chain in a dilute solution use the Zimm model on the next page; with a vanishing hydrodynamic interaction it is this model.
- Choose the number of beads `N` and the bond length `b` so that {math}`N b^2` equals the mean-square end-to-end distance of the real chain, the quantity that [chain statistics](../chem_02_polymers/chain_statistics.md) describes, and the friction `zeta` from the viscosity of the medium. The slow modes `p << N` are then independent of how finely you subdivide.
- A set of frictions, for example one per temperature, goes in as one Series: `rouse_relaxation_times` returns one row of `N - 1` times for each, and `rouse_modulus` returns one modulus curve for each. A set of chain lengths is a loop, because the number of modes changes with `N`.
- The model has no entanglement. For long chains in a dense melt the measured viscosity grows faster than {math}`N^2`, and this model underestimates it.

## Pitfalls

The functions check their inputs. A chain needs two beads, and the friction, bond length and thermal energy must be positive numbers; they are refused with a message instead of silently producing infinite times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace pd = chemistry::polymer_dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const auto zeta = vec({1e-12});
  // a bead-spring chain needs at least two beads
  try { pd::rouse_relaxation_times(1, zeta, 1e-9, 4.114e-21); } catch (const pluss::Error &e) { out.writeln("one bead: ", e.message()); }
  // friction and thermal energy have to be positive
  try { pd::rouse_relaxation_times(20, vec({0.0}), 1e-9, 4.114e-21); } catch (const pluss::Error &e) { out.writeln("zero friction: ", e.message()); }
  try { pd::rouse_relaxation_times(20, zeta, 1e-9, -4.114e-21); } catch (const pluss::Error &e) { out.writeln("negative kT: ", e.message()); }
  try { pd::rouse_relaxation_times(20, zeta, 0.0, 4.114e-21); } catch (const pluss::Error &e) { out.writeln("zero bond length: ", e.message()); }
  try { pd::rouse_diffusion(20, vec({-1e-12}), 4.114e-21); } catch (const pluss::Error &e) { out.writeln("negative friction (D): ", e.message()); }
}
```

Output

```text
one bead: natsci: rouse: need at least two beads
zero friction: natsci: rouse: friction must be strictly positive
negative kT: natsci: rouse: thermal energy must be finite and strictly positive
zero bond length: natsci: rouse: bond length must be finite and strictly positive
negative friction (D): natsci: rouse: friction must be strictly positive
```

Every mistake is reported by name: one bead, a zero friction, a negative thermal energy, a zero bond length, a negative friction in the diffusion call.

A chain of `N` beads has `N - 1` relaxing modes, because the zeroth mode is the translation of the whole chain, so the modulus at `t = 0` is {math}`c k_B T (N-1)`, not {math}`c k_B T N`. Counting beads and bonds the wrong way round shifts every result by one mode.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace pd = chemistry::polymer_dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // a chain of N beads has N - 1 internal modes, because the zeroth mode is the translation of the whole chain.
  // At t = 0 every one of them still carries kT, so G(0) = c kT (N - 1): the modulus is 49 and not 50 for 50 beads.
  const auto zeta = vec({1.0});
  const auto tau = pd::rouse_relaxation_times(50, zeta, 1.0, 1.0);
  out.writeln("number of relaxation times  ", tau.size(-1));
  out.writeln("G(0) / (c kT)               ", flat(pd::rouse_modulus(vec({0.0}), tau, 1.0, 1.0)));
  // the same chain described by 49 bonds instead of 50 beads has one mode fewer
  const auto tau49 = pd::rouse_relaxation_times(49, zeta, 1.0, 1.0);
  out.writeln("G(0) / (c kT), 49 beads     ", flat(pd::rouse_modulus(vec({0.0}), tau49, 1.0, 1.0)));
}
```

Output

```text
number of relaxation times  49
G(0) / (c kT)               [49.00]
G(0) / (c kT), 49 beads     [48.00]
```

`50` beads give `49` relaxation times and `G(0) / (c kT) = 49`; describing the same chain by `49` beads gives `48`. Decide whether your `N` counts beads or bonds before you read the modulus.

## Where next

The next page, [Zimm hydrodynamics](zimm_hydrodynamics.md), adds the hydrodynamic coupling between the beads that matters in a dilute solution. [Chain statistics](../chem_02_polymers/chain_statistics.md) shows how to choose the bead number and bond length from a measured molar mass.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="zimm_hydrodynamics.html">
    <span class="label">Next</span>
    <span class="title">Zimm hydrodynamics</span>
  </a>
</div>
<!-- apiref:nav:end -->
