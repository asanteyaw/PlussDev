# Diatomic partition functions

[Back to Chemistry 7: Spectroscopy](../chem_07_spectroscopy.md)

The heat capacity of a gas is set by how its molecules can store energy, and for a diatomic molecule the spectroscopic constants fix that completely. You want the rotational and vibrational partition functions of a molecule from its spectrum, and the heat capacity that follows. This page evaluates them from the four constants of the anharmonic oscillator and the non-rigid rotor, for carbon monoxide and nitrogen.

## How it works

A diatomic molecule is described by the harmonic frequency {math}`\omega_e`, its anharmonicity {math}`\omega_e x_e`, the rotational constant {math}`B_e` and the vibration-rotation coupling {math}`\alpha_e` (all in cm{math}`^{-1}`), with a symmetry number {math}`\sigma` that is {math}`2` for a homonuclear molecule. The vibrational levels are {math}`G(v) = \omega_e(v+\tfrac12) - \omega_e x_e (v+\tfrac12)^2` up to the point where the polynomial turns over, the rotational levels in vibrational state {math}`v` are {math}`B_v J(J+1)` with {math}`B_v = B_e - \alpha_e(v + \tfrac12)` and degeneracy {math}`(2J+1)/\sigma`. For each degree of freedom the partition function and the heat capacity are sums over levels,

```{math}
\begin{aligned}
&q = \sum_i g_i e^{-\varepsilon_i/k_BT}, \\
&\frac{c_v}{R} = \langle x^2\rangle - \langle x\rangle^2, \\
&x = \frac{\varepsilon}{k_BT}
\end{aligned}
```

`spectroscopy::rotational(d, T)` and `vibrational(d, T)` return the partition function `q`, the mean energy `mean` in units of {math}`k_BT` and the heat-capacity contribution `var`, over a Series of temperatures; `cp_over_r(d, T)` adds the translation and the {math}`PV` work, {math}`\tfrac52`, to give {math}`c_p/R` for the gas. Levels are counted up to {math}`J = 200` and until the vibrational ladder turns over, which is far beyond any thermally populated level for the temperatures a gas phase code uses.

## Partition functions of carbon monoxide

The constants of CO are {math}`\omega_e = 2169.8`, {math}`\omega_e x_e = 13.29`, {math}`B_e = 1.9313` and {math}`\alpha_e = 0.0175` cm{math}`^{-1}`. The program prints the rotational and vibrational partition functions and heat capacities at four temperatures.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace sp = pluss::ma::natsci::chemistry::spectroscopy;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // carbon monoxide: constants in cm^-1
  sp::Diatomic co;
  co.we = 2169.8; co.wexe = 13.29; co.be = 1.9313; co.alphae = 0.0175;
  const auto t = vec({100.0, 300.0, 1000.0, 2000.0});
  out.writeln("temperature (K)              ", flat(t));
  const auto rot = sp::rotational(co, t);
  out.writeln("rotational partition function ", flat(rot.q));
  out.writeln("rotational cv / R             ", flat(rot.var));
  const auto vib = sp::vibrational(co, t);
  out.writeln("vibrational partition function ", flat(vib.q));
  out.writeln("vibrational cv / R             ", flat(vib.var));
}
```

Output

```text
temperature (K)              [100.0000, 300.0000, 1000.0000, 2000.0000]
rotational partition function [36.4869, 108.7891, 361.8507, 723.3677]
rotational cv / R             [1.0000, 1.0000, 1.0000, 1.0000]
vibrational partition function [1.0000, 1.0000, 1.0481, 1.2741]
vibrational cv / R             [0.0000, 0.0036, 0.4809, 0.8370]
```

The rotational partition function grows linearly with temperature, `36.4869`, `108.7891`, `361.8507` and `723.3677` at {math}`100`, {math}`300`, {math}`1000` and {math}`2000` K, as {math}`q \approx k_BT/(hcB)` predicts for a rotor with many levels populated, and the rotational heat capacity has already reached its classical value `1.0000` {math}`R` at {math}`100` K. The vibrational partition function is still `1.0000` at {math}`300` K: only the ground state is occupied, so the vibrational heat capacity is `0.0036` {math}`R`. It rises to `0.4809` at {math}`1000` K and `0.8370` at {math}`2000` K as the mode is excited.

## The heat capacity of CO and N2

Nitrogen has {math}`\omega_e = 2358.6`, {math}`\omega_e x_e = 14.3`, {math}`B_e = 1.998`, {math}`\alpha_e = 0.0173` and symmetry number {math}`2`. The program prints {math}`c_p/R` for both gases over five temperatures, and the ratio of their rotational partition functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace sp = pluss::ma::natsci::chemistry::spectroscopy;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto t = vec({50.0, 100.0, 300.0, 1000.0, 2000.0});
  sp::Diatomic co;
  co.we = 2169.8; co.wexe = 13.29; co.be = 1.9313; co.alphae = 0.0175;
  sp::Diatomic n2;
  n2.we = 2358.6; n2.wexe = 14.3; n2.be = 1.998; n2.alphae = 0.0173; n2.symmetry = 2.0;
  out.writeln("temperature (K)  ", flat(t));
  out.writeln("cp/R of CO       ", flat(sp::cp_over_r(co, t)));
  out.writeln("cp/R of N2       ", flat(sp::cp_over_r(n2, t)));
  // the symmetry number changes the partition function by exactly a factor of two at high T
  out.writeln("q_rot(CO) / q_rot(N2) at 300, 1000 K  ", flat(sp::rotational(co, vec({300.0, 1000.0})).q.div(sp::rotational(n2, vec({300.0, 1000.0})).q)));
}
```

Output

```text
temperature (K)  [50.0000, 100.0000, 300.0000, 1000.0000, 2000.0000]
cp/R of CO       [3.5001, 3.5000, 3.5036, 3.9809, 4.3370]
cp/R of N2       [3.5001, 3.5000, 3.5018, 3.9244, 4.3072]
q_rot(CO) / q_rot(N2) at 300, 1000 K  [2.0693, 2.0694]
```

Both gases have {math}`c_p/R = 3.5` at {math}`50` and {math}`100` K: three translational and two rotational degrees of freedom plus the work term. At {math}`1000` K vibration has added `0.4809` for CO (`3.9809`) and slightly less for N{math}`_2` (`3.9244`), because N{math}`_2` has the stiffer bond and a higher vibrational frequency. The ratio of the rotational partition functions, `2.0693`, is almost the symmetry number {math}`2`, the small excess being the different rotational constants: the factor {math}`\sigma` is what makes N{math}`_2` rotate half as many distinct states as the heteronuclear CO.

## Decisions

- Use `rotational` and `vibrational` separately when you need entropy or partition functions, and `cp_over_r` when only the heat capacity matters.
- Set the symmetry number to {math}`2` for homonuclear molecules. It halves the rotational partition function and shifts the rotational entropy by {math}`R\ln 2`.
- Pass a Series of temperatures; the level sums are evaluated for every lane at once, so a table over temperature is one call.

## Pitfalls

The symmetry number is the high-temperature treatment of nuclear-spin statistics. The program evaluates the rotational partition function and heat capacity of N{math}`_2` at very low temperatures, and the vibrational partition function of a very anharmonic oscillator.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace sp = pluss::ma::natsci::chemistry::spectroscopy;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  sp::Diatomic n2;
  n2.we = 2358.6; n2.wexe = 14.3; n2.be = 1.998; n2.alphae = 0.0173; n2.symmetry = 2.0;
  // rotational heat capacity of N2 across the temperature at which only a few levels are populated
  const auto t = vec({2.0, 5.0, 10.0, 20.0, 100.0});
  out.writeln("temperature (K)         ", flat(t));
  out.writeln("rotational cv / R       ", flat(sp::rotational(n2, t).var));
  out.writeln("rotational q            ", flat(sp::rotational(n2, t).q));
  // a vibrational constant that is too anharmonic: the ladder stops where the polynomial turns over
  sp::Diatomic soft;
  soft.we = 100.0; soft.wexe = 10.0; soft.be = 1.0; soft.alphae = 0.01;
  out.writeln("soft oscillator, vibrational q at 100, 300, 1000 K  ", flat(sp::vibrational(soft, vec({100.0, 300.0, 1000.0})).q));
}
```

Output

```text
temperature (K)         [2.0000, 5.0000, 10.0000, 20.0000, 100.0000]
rotational cv / R       [1.0757, 1.0132, 1.0023, 1.0005, 1.0000]
rotational q            [0.5862, 1.0617, 1.9237, 3.6654, 17.6365]
soft oscillator, vibrational q at 100, 300, 1000 K  [1.6373, 3.3805, 4.9806]
```

At {math}`2` K the rotational partition function of N{math}`_2` is `0.5862`, below one, which no partition function of a real system can be: the ground state has weight {math}`1/\sigma = 0.5`, and the heat capacity, `1.0757` {math}`R`, is above the classical value. The treatment is right only when many rotational levels are populated, `1.0000` from about {math}`100` K; at low temperature the symmetry of the nuclear wave function has to be handled level by level (ortho and para forms). The oscillator ladder is cut where {math}`G(v)` turns over, so a very anharmonic oscillator has finitely many levels and a bounded partition function, `4.9806` at {math}`1000` K; that is a model of dissociation, not a calculation of it.

## Where next

[Chemistry 4: Reaction Rates](../chem_04_rates.md) uses these partition functions in the rate theory of a reaction, and [Absorption and the Beer-Lambert law](absorption.md) is the measurement that supplies the spectroscopic constants.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="line_shapes.html">
    <span class="label">Previous</span>
    <span class="title">Line shapes and broadening</span>
  </a>
</div>
<!-- apiref:nav:end -->
