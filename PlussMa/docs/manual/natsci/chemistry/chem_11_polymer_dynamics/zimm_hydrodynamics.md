# Zimm hydrodynamics

[Back to Chemistry 11: Polymer dynamics](../chem_11_polymer_dynamics.md)

In a dilute solution a moving bead drags the solvent around it, and that flow pushes the other beads of the same chain along. The chain then moves more like a single coil of solvent-soaked material than like a string of independent beads. The Zimm model adds exactly this coupling to the Rouse chain. This page builds the coupling matrix, compares the relaxation times of a chain with and without it, and computes the diffusion coefficient and the hydrodynamic radius of a chain in water.

## How it works

The Rouse chain has bead friction `zeta` and bond length `b`. In the Zimm model the velocity of each bead also contains the flow caused by every other bead, through the preaveraged Oseen interaction

```{math}
\begin{aligned}
&H_{nm} = \delta_{nm} + \sqrt{2}\,\frac{h^*}{\sqrt{|n-m|}}\,(1-\delta_{nm}), \\
&h^* = \frac{\zeta}{\eta_s\, b\,\sqrt{12\pi^3}} ,
\end{aligned}
```

where {math}`\eta_s` is the solvent viscosity. The strength of the coupling is the single number {math}`h^*`: it is `0` for no hydrodynamic interaction and about `0.25` for a typical flexible chain. The relaxation times follow from the eigenvalues {math}`\lambda_p` of the product `H A` (`A` is the Rouse connectivity matrix) as

```{math}
\begin{aligned}
&\tau_p = \frac{\zeta b^2}{3 k_B T\,\lambda_p}, \\
&D = \frac{k_B T}{N^2 \zeta}\sum_{n,m} H_{nm}.
\end{aligned}
```

For long chains this gives {math}`\tau_p \propto N^{3/2}` instead of the Rouse {math}`N^2` and {math}`D \propto N^{-1/2}` instead of {math}`N^{-1}`. The product `H A` is not symmetric, so the spectrum is computed from the symmetric matrix {math}`L^T A L`, where {math}`H = L L^T` is the Cholesky factorisation; the two have the same eigenvalues.

The functions are `pluss::ma::natsci::chemistry::polymer_dynamics::zimm_hydrodynamic_matrix(n_beads, h_star)`, `zimm_eigenvalues(n_beads, h_star)` (the `N` eigenvalues, starting with the zero translation mode), `zimm_relaxation_times(n_beads, h_star, zeta, b, kt)` and `zimm_diffusion(n_beads, h_star, zeta, kt)`. The last two take plain numbers and return a Series and a number, so a sweep over parameters is a loop.

## The coupling matrix and the spectrum

The program prints the coupling matrix of a four-bead chain at {math}`h^* = 0.2`, and the four smallest nonzero eigenvalues of a `50`-bead chain at {math}`h^* = 0`, `0.15` and `0.25` next to the Rouse eigenvalues.

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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the hydrodynamic-interaction matrix of a four-bead chain at h* = 0.2: ones on the diagonal, sqrt(2) h* / sqrt|n - m| off it
  const auto h = pd::zimm_hydrodynamic_matrix(4, 0.2);
  for (int i = 0; i < 4; ++i) out.writeln("H row ", i, "  ", flat(h.select(0, i)));
  // the smallest eigenvalues of H A for a chain of 50 beads, in units zeta b^2 / (3 kT) = 1, at three interaction strengths
  for (const double hs : {0.0, 0.15, 0.25}) out.writeln("h* = ", hs, "   lambda_1..4 ", flat(pd::zimm_eigenvalues(50, hs).slice(-1, 1, 5, 1)));
  // with hs = 0 the spectrum is the Rouse spectrum
  out.writeln("Rouse            lambda_1..4 ", flat(pd::rouse_eigenvalues(50).slice(-1, 1, 5, 1)));
}
```

Output

```text
H row 0  [1.00000, 0.28284, 0.20000, 0.16330]
H row 1  [0.28284, 1.00000, 0.28284, 0.20000]
H row 2  [0.20000, 0.28284, 1.00000, 0.28284]
H row 3  [0.16330, 0.20000, 0.28284, 1.00000]
h* = 0.00000   lambda_1..4 [0.00395, 0.01577, 0.03543, 0.06283]
h* = 0.15000   lambda_1..4 [0.00838, 0.02772, 0.05453, 0.08798]
h* = 0.25000   lambda_1..4 [0.01126, 0.03554, 0.06710, 0.10456]
Rouse            lambda_1..4 [0.00395, 0.01577, 0.03543, 0.06283]
```

The off-diagonal entries are {math}`\sqrt{2}h^*/\sqrt{|n-m|}`: `0.28284` for neighbours, `0.20000` for beads two apart and `0.16330` for three apart, so the coupling falls off slowly with distance along the chain. With {math}`h^* = 0` the eigenvalues are the Rouse eigenvalues, `0.00395, 0.01577, ...`, and each increase of the interaction raises them: the slowest mode has `0.00838` at `0.15` and `0.01126` at `0.25`, which means shorter relaxation times.

## Rouse against Zimm as the chain grows

The slowest relaxation time in units of {math}`\zeta b^2/(3k_BT)`, for chains of `25` to `200` beads, for the Rouse model and for the Zimm model at {math}`h^* = 0.25`, and the exponent with which each grows between `200` and `400` beads.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // relaxation times of the slowest mode in units zeta b^2 / (3 kT): Rouse against Zimm (h* = 0.25) as the chain grows
  const auto zeta = vec({3.0});  // so that zeta b^2 / (3 kT) = 1 with b = kT = 1
  for (const std::int64_t n : {25, 50, 100, 200}) {
    const double rouse = flat(pd::rouse_relaxation_times(n, zeta, 1.0, 1.0))[0];
    const double zimm = flat(pd::zimm_relaxation_times(n, 0.25, 3.0, 1.0, 1.0))[0];
    out.writeln("N = ", n, "   tau_1 Rouse = ", rouse, "   Zimm = ", zimm, "   ratio = ", rouse / zimm);
  }
  // the exponent of the growth of tau_1 with chain length: 2 for Rouse, about 3/2 for Zimm
  const auto exponent = [&](bool zimm) {
    const auto t = [&](std::int64_t n) { return zimm ? flat(pd::zimm_relaxation_times(n, 0.25, 3.0, 1.0, 1.0))[0] : flat(pd::rouse_relaxation_times(n, zeta, 1.0, 1.0))[0]; };
    return std::log(t(400) / t(200)) / std::log(2.0);
  };
  out.writeln("exponent of tau_1 between N = 200 and 400:  Rouse ", exponent(false), "   Zimm ", exponent(true));
}
```

Output

```text
N = 25   tau_1 Rouse = 63.4091   Zimm = 31.3586   ratio = 2.0221
N = 50   tau_1 Rouse = 253.3863   Zimm = 88.7767   ratio = 2.8542
N = 100   tau_1 Rouse = 1013.2952   Zimm = 251.0249   ratio = 4.0366
N = 200   tau_1 Rouse = 4052.9307   Zimm = 709.4288   ratio = 5.7129
exponent of tau_1 between N = 200 and 400:  Rouse 2.0000   Zimm 1.4987
```

The Rouse time is `63.4` for `25` beads and quadruples with every doubling; the Zimm time is `31.4`, and the ratio between the two grows from `2.02` to `5.71` as the chain lengthens, because a longer coil shields its interior from the solvent more effectively. The fitted exponents are `2.0000` for Rouse and `1.4987` for Zimm, the {math}`3/2` that the theory predicts.

## A chain in water

A polymer in water at `298` K: bond length `0.5` nm, solvent viscosity {math}`8.9\times10^{-4}` Pa s and a bead of radius `0.125` nm, so that the bead friction is {math}`6\pi\eta_s a`. The program derives the interaction parameter from these numbers, then prints the diffusion coefficient of chains of `20`, `80` and `320` beads with and without hydrodynamic interaction, and the hydrodynamic radius {math}`k_BT/(6\pi\eta_s D)`.

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
  // a polymer in water at 298 K: bond length 0.5 nm, bead friction zeta = 6 pi eta_s a for a bead radius of 0.125 nm
  const double kt = 4.114e-21, eta_s = 8.9e-4, a = 0.125e-9, b = 0.5e-9;
  const double zeta = 6.0 * std::numbers::pi * eta_s * a;
  // the interaction parameter that follows from these numbers: h* = zeta / (eta_s b sqrt(12 pi^3))
  const double h_star = zeta / (eta_s * b * std::sqrt(12.0 * std::pow(std::numbers::pi, 3.0)));
  out.writeln("zeta (kg/s) = ", zeta, "   h* = ", h_star);
  // diffusion of the whole chain with and without hydrodynamic interaction, and the hydrodynamic radius kT / (6 pi eta_s D)
  for (const std::int64_t n : {20, 80, 320}) {
    const double d_zimm = pd::zimm_diffusion(n, h_star, zeta, kt);
    const double d_rouse = flat(pd::rouse_diffusion(n, vec({zeta}), kt))[0];
    out.writeln("N = ", n, "   D Rouse = ", d_rouse, "   D Zimm = ", d_zimm, "   R_h (nm) = ", kt / (6.0 * std::numbers::pi * eta_s * d_zimm) * 1e9);
  }
}
```

Output

```text
zeta (kg/s) = 2.0970e-12   h* = 2.4430e-01
N = 20   D Rouse = 9.8092e-11   D Zimm = 4.0391e-10   R_h (nm) = 6.0713e-01
N = 80   D Rouse = 2.4523e-11   D Zimm = 2.0190e-10   R_h (nm) = 1.2146e+00
N = 320   D Rouse = 6.1307e-12   D Zimm = 1.0099e-10   R_h (nm) = 2.4283e+00
```

The numbers give {math}`h^* = 0.244`, close to the typical `0.25`. With the interaction, the diffusion coefficient falls by a factor of `2` for every fourfold increase of the chain length (`4.04e-10`, `2.02e-10`, `1.01e-10` m²/s), the {math}`N^{-1/2}` law, while the Rouse chain falls by a factor of `4`. At `320` beads the Rouse value is `16` times too small. The hydrodynamic radius doubles from `0.61` to `1.21` to `2.43` nm for each fourfold increase in length, which is how a coil's size grows with the square root of its length.

## Decisions

- Take {math}`h^*` from your own friction and solvent through the relation above, or use `0.25` when the chain is a generic flexible one. A theta solvent and a good solvent differ by the chain statistics, not by this parameter in the model.
- The interaction can only be as strong as the coupling matrix allows: `H` has to stay positive definite, which for chains of `20` to `320` beads fails somewhere between {math}`h^* = 0.5` and `0.6`. Stay at or below `0.5`; a larger value is refused with a message that names it.
- The hydrodynamic radius from the diffusion coefficient, {math}`k_BT/(6\pi\eta_s D)`, is the number to compare with a measured one, since light scattering and diffusion experiments report it directly.

## Pitfalls

The interaction parameter is checked: it has to be non-negative, and small enough for the coupling matrix to remain positive definite. The friction, bond length and thermal energy have to be positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace pd = chemistry::polymer_dynamics;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the interaction parameter has to stay below the value at which the hydrodynamic matrix stops being positive definite
  for (const double hs : {0.5, 1.0}) {
    try {
      out.writeln("h* = ", hs, "  D N zeta / kT = ", pd::zimm_diffusion(30, hs, 1.0, 1.0) * 30.0);
    } catch (const pluss::Error &e) { out.writeln("h* = ", hs, "  ", e.message()); }
  }
  try { (void)pd::zimm_eigenvalues(30, -0.1); } catch (const pluss::Error &e) { out.writeln("negative h*: ", e.message()); }
  try { (void)pd::zimm_relaxation_times(30, 0.15, 0.0, 1.0, 1.0); } catch (const pluss::Error &e) { out.writeln("zero friction: ", e.message()); }
}
```

Output

```text
h* = 0.5000  D N zeta / kT = 9.2718
h* = 1.0000  natsci::chemistry::polymer_dynamics: the preaveraged hydrodynamic interaction is not positive definite for h* = 1.000000 and 30 beads; use a smaller h*
negative h*: natsci: zimm: the hydrodynamic-interaction parameter h* must be finite and non-negative
zero friction: natsci: zimm: friction must be finite and strictly positive
```

`h* = 0.5` is accepted for a chain of `30` beads, `1.0` is refused with a message that says the interaction is not positive definite and which `h*` and bead count were asked for, a negative `h*` and a zero friction are refused by name.

## Where next

Return to [Rouse chains](rouse_chains.md) for the viscosity and the stress relaxation of a melt, where hydrodynamics is screened. [Chain statistics](../chem_02_polymers/chain_statistics.md) relates the bond length and bead number to a measured molar mass.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rouse_chains.html">
    <span class="label">Previous</span>
    <span class="title">Rouse chains</span>
  </a>
</div>
<!-- apiref:nav:end -->
