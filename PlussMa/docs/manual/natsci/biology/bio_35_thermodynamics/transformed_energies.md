# Transformed energies and equilibria

[Back to Biology 35: Biochemical Thermodynamics](../bio_35_thermodynamics.md)

A reaction in a cell runs at a fixed pH and ionic strength, where a metabolite exists as a mixture of protonation states and the activities of ions differ from their concentrations. You want the Gibbs energy and equilibrium constant that apply there, and how they move with pH, ionic strength and temperature. This page computes the transformed formation energy of a group of protonation states, the energy and equilibrium constant of a reaction at a chosen pH, the ionic-strength corrections, and the energy of a group of microspecies that includes magnesium complexes.

## How it works

Take a reactant with $n$ hydrogens in its fully protonated form and stepwise acid constants $\mathrm{p}K_1,\dots,\mathrm{p}K_n$ (only the ones that matter in the range of interest). The states of the reactant at a given pH form a *pseudoisomer group*, with the *binding polynomial*

```{math}
P(\mathrm{pH}) = \sum_{j=0}^{n} 10^{\,j\,\mathrm{pH} - \sum_{m\le j}\mathrm{p}K_m}
```

and the transformed formation energy of the group, after the Legendre transform that fixes the proton potential, is

```{math}
\Delta_f G'(\text{group}) = \Delta_f G(\mathrm{H}_n\mathrm{A}) + n\,RT\ln 10\ \mathrm{pH} - RT\ln P
```

In the library, `thermodynamics::proton_binding_polynomial(ph, pka)` gives $P$, `transformed_formation_gibbs(dgf, n, pka, ph, T)` gives the transformed energy in J/mol from the formation energy `dgf` of the fully protonated form (a species that cannot change its protonation in range has `transformed_formation_gibbs_fixed`). For a reaction with coefficients $\nu_i$, `reaction_gibbs(dgf_transformed, nu)` sums them to $\Delta_r G'^{\circ}$, `apparent_equilibrium_constant(dgr, T)` returns $K' = \exp(-\Delta_r G'^{\circ}/RT)$, and `reaction_gibbs_at(dgr, ln_q, T)` gives $\Delta_r G' = \Delta_r G'^{\circ} + RT\ln Q$. Activity corrections use the extended Debye-Hückel form $\ln\gamma = -\alpha z^2\sqrt I/(1 + B\sqrt I)$, `debye_huckel_ln_gamma(charge, I, alpha, B)`, and the temperature-dependent coefficient `debye_huckel_factor(I, T)` in J/mol. A group of microspecies with different charge, protons and magnesium is described by a list of `Microspecies` and `transformed_group_gibbs(species, ph, pMg, I, T)`. All energies are in J/mol, temperatures in kelvin, and the energies of formation in the programs are illustrative numbers, not data.

## How the binding polynomial counts species

The program computes $P$ for an acid with $\mathrm{p}K = 6.5$ and for a two-step acid at five values of pH, and the fraction of the acid that is fully protonated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an acid with one dissociable proton and pKa = 6.5: the binding polynomial P = 1 + 10^(pH - pKa) counts the species at a given pH
  const auto ph = vec({4.5, 5.5, 6.5, 7.5, 8.5});
  out.writeln("pH          ", flat(ph));
  out.writeln("P (pKa 6.5) ", flat(thermo::proton_binding_polynomial(ph, vec({6.5}))));
  // a diprotic acid (pKa 2.2 and 7.2, like phosphate's second and third steps shifted): P = 1 + 10^(pH - 2.2) + 10^(2 pH - 9.4)
  out.writeln("P (pKa 2.2, 7.2) ", flat(thermo::proton_binding_polynomial(ph, vec({2.2, 7.2}))));
  // the fraction of the fully protonated form is 1 / P
  out.writeln("fraction protonated, pKa 6.5 ", flat(thermo::proton_binding_polynomial(ph, vec({6.5})).reciprocal()));
}
```

Output

```text
pH          [4.5000, 5.5000, 6.5000, 7.5000, 8.5000]
P (pKa 6.5) [1.0100, 1.1000, 2.0000, 11.0000, 101.0000]
P (pKa 2.2, 7.2) [200.9243, 2036.0730, 23934.6949, 597634.4021, 41805980.3703]
fraction protonated, pKa 6.5 [0.9901, 0.9091, 0.5000, 0.0909, 0.0099]
```

For the single acid $P = 1 + 10^{\mathrm{pH} - 6.5}$ is `1.01` at pH `4.5` and `2.0` at $\mathrm{pH} = \mathrm{p}K$, where the acid is half protonated (`0.5000`), and `101` at pH `8.5`. The fraction protonated, $1/P$, falls from `0.9901` to `0.0099`. For the two-step acid $P$ is the three-term sum, and grows by factors of `10` to `100` per pH unit once both steps are active.

## From formation energy to the transformed energy

The program computes the transformed formation energy of an acid with a formation energy of $-500$ kJ/mol over six pH values, compares it with the same species held at fixed protonation, and prints $-RT\ln P$.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double rt_kj = 8.314462618e-3 * 298.15;   // R T in kJ/mol at 298.15 K
  out.writeln("RT at 298.15 K, kJ/mol ", std::vector<double>{rt_kj});
  // a monoprotic acid HA with formation energy -500 kJ/mol (an illustrative number), pKa 6.5, one hydrogen in the protonated form
  const auto ph = vec({5.0, 6.0, 6.5, 7.0, 8.0, 9.0});
  const auto group = thermo::transformed_formation_gibbs(vec({-500000.0}), 1.0, vec({6.5}), ph, 298.15);
  out.writeln("pH                                    ", flat(ph));
  out.writeln("transformed formation energy, kJ/mol  ", flat(group.div(1000.0)));
  // a species that cannot lose its proton in this range has only the Legendre term N_H R T ln10 pH: a straight line in pH
  const auto fixed = thermo::transformed_formation_gibbs_fixed(vec({-500000.0}), 1.0, ph, 298.15);
  out.writeln("same, fixed protonation, kJ/mol       ", flat(fixed.div(1000.0)));
  out.writeln("difference group - fixed, kJ/mol      ", flat(group.sub(fixed).div(1000.0)));
  out.writeln("-RT ln P, kJ/mol (the same thing)     ", flat(thermo::proton_binding_polynomial(ph, vec({6.5})).log().mul(-rt_kj)));
}
```

Output

```text
RT at 298.15 K, kJ/mol [2.4790]
pH                                    [5.0000, 6.0000, 6.5000, 7.0000, 8.0000, 9.0000]
transformed formation energy, kJ/mol  [-471.5371, -466.4331, -464.6162, -463.5791, -462.9751, -462.9058]
same, fixed protonation, kJ/mol       [-471.4600, -465.7519, -462.8979, -460.0439, -454.3359, -448.6279]
difference group - fixed, kJ/mol      [-0.0772, -0.6811, -1.7183, -3.5351, -8.6392, -14.2779]
-RT ln P, kJ/mol (the same thing)     [-0.0772, -0.6811, -1.7183, -3.5351, -8.6392, -14.2779]
```

At $298.15$ K, $RT = 2.4790$ kJ/mol. A species that cannot change protonation has a transformed energy that rises by $RT\ln 10 = 5.71$ kJ/mol for each pH unit, a straight line, from `-471.46` at pH `5` to `-448.63` at pH `9`. The group, which can shed its proton, rises less steeply and flattens at high pH (`-462.98` and `-462.91` at pH `8` and `9`), because above the $\mathrm{p}K$ the deprotonated form takes over and has no proton term. The difference group minus fixed is exactly $-RT\ln P$, as the last line shows: `-0.0772` at pH `5` and `-14.2779` at pH `9`.

## A reaction at a given pH

The program takes an isomerisation $A\to B$ of two acids with different $\mathrm{p}K$ values (`6.5` and `8.0`), computes $\Delta_r G'^{\circ}$ and the equilibrium constant over five pH values, and then the actual reaction energy at a concentration ratio $[B]/[A] = 0.01$.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double t = 298.15;
  // an isomerisation  A <-> B  of two acids with different pKa (energies illustrative): A has pKa 6.5 and B has pKa 8.0
  const auto ph = vec({5.0, 6.0, 7.0, 8.0, 9.0});
  const auto ga = thermo::transformed_formation_gibbs(vec({-500000.0}), 1.0, vec({6.5}), ph, t);
  const auto gb = thermo::transformed_formation_gibbs(vec({-498000.0}), 1.0, vec({8.0}), ph, t);
  const auto g = ga.unsqueeze(-1).unite(std::vector<pluss::Series>{gb.unsqueeze(-1)}, 1);   // [pH, 2]
  const auto dg = thermo::reaction_gibbs(g, vec({-1.0, 1.0}));                              // formation energy of B minus that of A
  out.writeln("pH                              ", flat(ph));
  out.writeln("transformed Delta_r G'o, kJ/mol ", flat(dg.div(1000.0)));
  out.writeln("apparent equilibrium constant   ", flat(thermo::apparent_equilibrium_constant(dg, t)));
  // away from equilibrium: with [B]/[A] = 0.01 the reaction energy Delta_r G' = Delta_r G'o + R T ln Q
  const auto ln_q = vec({std::log(0.01)}).select(0, 0);
  out.writeln("Delta_r G' at Q = 0.01, kJ/mol  ", flat(thermo::reaction_gibbs_at(dg, ln_q, t).div(1000.0)));
}
```

Output

```text
pH                              [5.0000, 6.0000, 7.0000, 8.0000, 9.0000]
transformed Delta_r G'o, kJ/mol [2.0747, 2.6565, 5.2989, 8.9209, 10.3336]
apparent equilibrium constant   [0.4330, 0.3425, 0.1179, 0.0274, 0.0155]
Delta_r G' at Q = 0.01, kJ/mol  [-9.3413, -8.7595, -6.1171, -2.4951, -1.0824]
```

The standard energy of the reaction rises from `2.07` kJ/mol at pH `5` to `10.33` kJ/mol at pH `9`, and the equilibrium constant falls from `0.4330` to `0.0155`: as the pH rises, the acid A (the one with the lower $\mathrm{p}K$) loses its proton first and is stabilised, which favours the reactant. At a ratio $Q = 0.01$ the reaction energy $\Delta_r G' = \Delta_r G'^{\circ} + RT\ln Q$ is `-9.34` at pH `5` and `-1.08` at pH `9`, so the reaction is downhill for this ratio at every pH shown.

## Ionic strength and temperature

The program evaluates the Debye-Hückel activity coefficient for charges `1`, `2` and `3`, the correction term over five ionic strengths, its temperature dependence and the formation energy of the magnesium ion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // extended Debye-Hueckel: ln gamma = -alpha z^2 sqrt(I) / (1 + B sqrt(I)), here with alpha = 1.17582 and B = 1.6
  const auto charge = vec({1.0, 2.0, 3.0});
  const auto ionic = vec({0.1});
  out.writeln("ln gamma for charges 1, 2, 3 at I = 0.1 M ", flat(thermo::debye_huckel_ln_gamma(charge, ionic, 1.17582)));
  // ionic strength dependence of the correction term DH(I, T), in J/mol, at 298.15 K
  const auto strengths = vec({0.0, 0.05, 0.1, 0.25, 0.5});
  out.writeln("ionic strength, M     ", flat(strengths));
  out.writeln("DH(I, 298.15 K), J/mol ", flat(thermo::debye_huckel_factor(strengths, 298.15)));
  // temperature dependence at I = 0.1 M
  for (double t : {278.15, 298.15, 318.15})
    out.writeln("T = ", std::vector<double>{t}, " K   DH(0.1 M) ", flat(thermo::debye_huckel_factor(vec({0.1}), t)));
  // the formation energy of Mg2+ at three temperatures, kJ/mol
  out.writeln("Delta_f G(Mg2+), kJ/mol at 278.15, 298.15, 318.15 K ", std::vector<double>{thermo::magnesium_formation_gibbs(278.15) / 1000.0, thermo::magnesium_formation_gibbs(298.15) / 1000.0, thermo::magnesium_formation_gibbs(318.15) / 1000.0});
}
```

Output

```text
ln gamma for charges 1, 2, 3 at I = 0.1 M [-0.2469, -0.9876, -2.2221]
ionic strength, M     [0.0000, 0.0500, 0.1000, 0.2500, 0.5000]
DH(I, 298.15 K), J/mol [0.0000, 480.0434, 612.0788, 809.6914, 967.0479]
T = [278.1500] K   DH(0.1 M) [552.6903]
T = [298.1500] K   DH(0.1 M) [612.0788]
T = [318.1500] K   DH(0.1 M) [676.7498]
Delta_f G(Mg2+), kJ/mol at 278.15, 298.15, 318.15 K [-456.0848, -455.3000, -454.5152]
```

At $I = 0.1$ M, $\ln\gamma$ is `-0.2469`, `-0.9876` and `-2.2221` for charges `1`, `2`, `3`: it grows with the square of the charge, so highly charged species are stabilised most. The correction term rises with ionic strength but more slowly as it grows, `480` J/mol at `0.05` M and `967` J/mol at `0.5` M, because of the $1 + B\sqrt I$ in the denominator. It rises with temperature, `553`, `612`, `677` J/mol at `278`, `298` and `318` K, and the energy of formation of $\mathrm{Mg}^{2+}$ rises from `-456.08` to `-454.52` kJ/mol over the same range.

## A group of microspecies

The program describes a metabolite as two microspecies, a neutral form with one proton and a charged form with none that lies `5` $RT$ higher, and computes the group energy at three values of pH, the major form alone and the Legendre term of the charged form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double t = 298.15;
  const auto ph = vec({6.0, 7.0, 8.0});
  const auto ionic = vec({0.1});
  // one metabolite as a group of two microspecies: the neutral major form and a deprotonated form (charge -1, one proton fewer) lying 5.0 RT higher
  std::vector<thermo::Microspecies> group;
  group.push_back({0.0, 1.0, 0.0, 0.0});
  group.push_back({-1.0, 0.0, 0.0, 5.0});
  const auto both = thermo::transformed_group_gibbs(group, ph, 3.0, ionic, t);
  out.writeln("pH                                            ", flat(ph));
  out.writeln("group energy relative to the major form, kJ/mol ", flat(both.div(1000.0)));
  // the major microspecies alone
  const auto major = thermo::transformed_group_gibbs({group[0]}, ph, 3.0, ionic, t);
  out.writeln("major microspecies alone, kJ/mol               ", flat(major.div(1000.0)));
  // the contribution of the Legendre transform of the second microspecies at the three pH
  out.writeln("Legendre term of the charged species, kJ/mol   ", flat(thermo::microspecies_legendre_transform(-1.0, 0.0, 0.0, ph, 3.0, ionic, t).div(1000.0)));
}
```

Output

```text
pH                                            [6.0000, 7.0000, 8.0000]
group energy relative to the major form, kJ/mol [11.7825, 11.7827, 11.7827]
major microspecies alone, kJ/mol               [34.8601, 40.5681, 46.2762]
Legendre term of the charged species, kJ/mol   [-0.6121, -0.6121, -0.6121]
```

The group energy is `11.78` kJ/mol at every pH shown, while the major microspecies alone would be `34.86`, `40.57` and `46.28` kJ/mol: at these pH values the charged form, which has no proton term, is the species that holds the group energy, and the group energy is the (pH-independent) energy of the charged form, $5RT$ plus its Legendre term `-0.6121`. The group energy is always lower than that of any one of its microspecies.

## Decisions

- Use `transformed_formation_gibbs` for a reactant whose protonation changes in your pH range and `transformed_formation_gibbs_fixed` for one that does not.
- Compute $K'$ and $\Delta_r G'$ from transformed energies at the pH and ionic strength of the experiment, never from standard energies at pH 0.
- Pass temperatures in kelvin and energies in J/mol throughout: the functions do not convert.
- Use `Microspecies` and `transformed_group_gibbs` when magnesium binding matters or the group has more than two species; the simple polynomial form covers protonation only.

## Pitfalls

Two inputs are easy to get wrong without any error: a temperature in degrees Celsius and the number of hydrogens. The program uses both wrongly, and then passes an empty group.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto ph = vec({7.0});
  const auto pka = vec({6.5});
  // a temperature given in degrees Celsius instead of kelvin: R T is a tenth of what it should be, and every energy in the group term is wrong
  const auto right = thermo::transformed_formation_gibbs(vec({-500000.0}), 1.0, pka, ph, 298.15);
  const auto wrong = thermo::transformed_formation_gibbs(vec({-500000.0}), 1.0, pka, ph, 25.0);
  out.writeln("at 298.15 K, kJ/mol ", flat(right.div(1000.0)));
  out.writeln("at 25 (Celsius given as kelvin), kJ/mol ", flat(wrong.div(1000.0)));
  // the number of protons enters twice: as n in the Legendre term and as the number of pKa values; they must be consistent
  const auto one_pka = thermo::transformed_formation_gibbs(vec({-500000.0}), 1.0, vec({6.5}), ph, 298.15);
  const auto two_h = thermo::transformed_formation_gibbs(vec({-500000.0}), 2.0, vec({6.5}), ph, 298.15);
  out.writeln("n = 1, one pKa, kJ/mol ", flat(one_pka.div(1000.0)));
  out.writeln("n = 2, one pKa, kJ/mol ", flat(two_h.div(1000.0)));
  // a group with no microspecies
  try {
    thermo::transformed_group_gibbs({}, ph, 3.0, vec({0.1}), 298.15);
  } catch (const pluss::Error &e) {
    out.writeln("empty group: ", e.message());
  }
}
```

Output

```text
at 298.15 K, kJ/mol [-463.5791]
at 25 (Celsius given as kelvin), kJ/mol [-496.9461]
n = 1, one pKa, kJ/mol [-463.5791]
n = 2, one pKa, kJ/mol [-423.6230]
empty group: natsci: thermodynamics: a group needs at least one microspecies
```

A temperature of `25` taken as kelvin gives `-496.95` kJ/mol instead of `-463.58`: an error of over `33` kJ/mol that nothing detects, because `25` K is a valid temperature. The number of hydrogens $n$ counts every hydrogen of the fully protonated form, not only the acidic ones: the same single $\mathrm{p}K$ with $n = 2$ gives `-423.62`, a different quantity. An empty group is refused with a message that says a group needs at least one microspecies.

## Where next

Continue with [Pathway feasibility](pathway_feasibility.md) to use these energies to ask whether a pathway can run, and see [Biology 4: Metabolism](../bio_04_metabolism.md) for the fluxes themselves.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="pathway_feasibility.html">
    <span class="label">Next</span>
    <span class="title">Pathway feasibility</span>
  </a>
</div>
<!-- apiref:nav:end -->
