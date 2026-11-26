# The next-generation matrix

[Back to Biology 30: Reproduction Numbers of Structured Epidemics](../bio_30_reproduction_numbers.md)

An epidemic spreads when each case causes, on average, more than one new case. You have a model with several infected compartments, such as exposed and infectious people, or people on and off treatment, and you want the threshold quantity $R_0$ without solving for it by hand. This page writes the model as a split into new infections and transfers, builds the next-generation matrix numerically, takes its dominant eigenvalue, and sweeps $R_0$ over parameters in one call.

## How it works

Let $x$ be the state, with the first $m$ compartments infected. The rate of change of the infected compartments splits as $\dot x_i = F_i(x) - V_i(x)$, where $F_i$ is the rate of appearance of new infections in compartment $i$ and $V_i$ is the net rate of transfer out of it by every other means (progression, recovery, death, minus the inflow from other infected compartments). At the disease-free equilibrium $x_0$, with Jacobians $F = \partial F/\partial x$ and $V = \partial V/\partial x$ restricted to the infected block, the *next-generation matrix* is

```{math}
\begin{aligned}
&K = F\,V^{-1}, \\
&R_0 = \rho(K)
\end{aligned}
```

and $\rho$ is the spectral radius. Entry $K_{ij}$ is the expected number of new infections in compartment $i$ caused by one individual who starts in compartment $j$, over that individual's whole infectious life. A `epidemiology::Split` holds the two functions `infections` ($F$) and `transfers` ($V$), the number `infected` of infected compartments and, optionally, their indices. `epidemiology::next_generation(split, dfe)` differentiates both numerically at the disease-free state and returns $K$; `epidemiology::reproduction_number(split, dfe)` returns $\rho(K)$. The library supplies splits for the treatment, staged, multi-strain and vector-host models, and the parameters may have leading axes, so one call can give $R_0$ for a whole table of settings. The treatment model has the states (E, I, S, T): exposed, infectious, susceptible and treated, with infection rates $\beta_1$ (untreated) and $\beta_2$ (treated), a latency rate $\nu$, treatment rates $r_1$ for exposed and $r_2$ for infectious people, a fraction $q$ of the treated infectious that stay treated, and births equal to deaths at rate $d$.

## R0 with and without treatment

The program computes $R_0$ of the treatment model from its split, compares it with the closed form $\beta_1\nu/\bigl((d+\nu+r_1)(d+r_2) - \nu(1-q)r_2\bigr)$, then switches treatment off and compares again with $\beta_1\nu/(d(d+\nu))$.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace epi = biology::epidemiology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // exposed (E), infectious (I), susceptible (S) and treated (T) people. Infection at beta1 (untreated) and beta2 (treated), latency rate nu,
  // treatment of exposed people at r1 and of infectious people at r2, a fraction q of the treated infectious stay treated, and births = deaths = d
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  const auto dfe = vec({0.0, 0.0, 1.0, 0.0});   // nobody infected, everybody susceptible
  const auto split = epi::treatment_split(p);
  out.writeln("R0 with treatment                  ", flat(epi::reproduction_number(split, dfe)));
  // the closed form: beta1 nu / ((d + nu + r1)(d + r2) - nu (1 - q) r2)
  out.writeln("closed form                        ", std::vector<double>{1.5 * 0.3 / ((0.05 + 0.3 + 0.2) * (0.05 + 0.35) - 0.3 * 0.3 * 0.35)});
  // switch the treatment off: R0 = beta1 nu / (d (d + nu))
  epi::TreatmentParams none = p;
  none.r1 = 0.0;
  none.r2 = 0.0;
  out.writeln("R0 without treatment               ", flat(epi::reproduction_number(epi::treatment_split(none), dfe)));
  out.writeln("closed form                        ", std::vector<double>{1.5 * 0.3 / (0.05 * 0.35)});
  // the DFE is an equilibrium: the rates there are zero
  out.writeln("largest rate at the disease-free state ", std::vector<double>{epi::treatment_rate(dfe, p).abs().max().item<double>()});
}
```

Output

```text
R0 with treatment                  [2.3873]
closed form                        [2.3873]
R0 without treatment               [25.7143]
closed form                        [25.7143]
largest rate at the disease-free state [0.0000]
```

The numerical $R_0$ is `2.3873` and the closed form agrees to the digits printed. Without treatment $R_0$ is `25.7143` and again equals the closed form: treatment cuts the reproduction number by a factor of more than ten. The last line shows that the disease-free state is an equilibrium of the full model: every rate there is `0.0000`.

## Reading the matrix

The program prints the next-generation matrix itself, with its rows for the exposed and infectious compartments, and computes its dominant eigenvalue from the two-by-two formula.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace epi = biology::epidemiology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the next-generation matrix F V^-1: entry (i, j) is the number of new cases in infected compartment i caused by one person in compartment j over its lifetime
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  const auto dfe = vec({0.0, 0.0, 1.0, 0.0});
  const auto k = epi::next_generation(epi::treatment_split(p), dfe);
  out.writeln("next-generation matrix (E, I rows) ", flat(k));
  // new infections appear only as exposed people (row E); the second row is zero. The dominant eigenvalue of K is R0
  const auto a = flat(k);
  const double trace = a[0] + a[3], det = a[0] * a[3] - a[1] * a[2];
  out.writeln("dominant eigenvalue from the 2x2 formula ", std::vector<double>{0.5 * (trace + std::sqrt(trace * trace - 4.0 * det))});
  out.writeln("reproduction_number                   ", flat(epi::reproduction_number(epi::treatment_split(p), dfe)));
}
```

Output

```text
next-generation matrix (E, I rows) [2.3873, 4.3767, 0.0000, 0.0000]
dominant eigenvalue from the 2x2 formula [2.3873]
reproduction_number                   [2.3873]
```

All the new infections appear as exposed people, so the second row of $K$ is zero, and $K = \begin{pmatrix} 2.3873 & 4.3767 \\ 0 & 0\end{pmatrix}$. One newly exposed person causes `2.3873` new exposed people through their own infectious period, and one infectious person causes `4.3767` (the exposed compartment of their infectees, reached with no latency). The dominant eigenvalue from the formula is `2.3873`, the same number that `reproduction_number` returns.

## A table of settings in one call

Parameters can carry a leading axis. The program gives `beta1` four values and, separately, the treatment rate `r2` five values, with one disease-free state for each setting, and reads $R_0$ for all of them in one call each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace epi = biology::epidemiology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a batch of settings in one call: R0 against the infection rate beta1 and against the treatment rate r2 of infectious people
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  const auto dfe = vec({0.0, 0.0, 1.0, 0.0});
  epi::TreatmentParams sweep = p;
  sweep.beta1 = vec({0.5, 1.0, 1.5, 2.0}).unsqueeze(-1);   // shape [4, 1] broadcasts against the [4, 4] states
  const auto lanes = dfe.unsqueeze(0).unite(std::vector<pluss::Series>{dfe.unsqueeze(0), dfe.unsqueeze(0), dfe.unsqueeze(0)}, 0);
  out.writeln("beta1                    ", std::vector<double>{0.5, 1.0, 1.5, 2.0});
  out.writeln("R0                       ", flat(epi::reproduction_number(epi::treatment_split(sweep), lanes)));
  epi::TreatmentParams rates = p;
  rates.r2 = vec({0.0, 0.2, 0.5, 1.0, 2.0}).unsqueeze(-1);
  const auto lanes5 = dfe.unsqueeze(0).unite(std::vector<pluss::Series>{dfe.unsqueeze(0), dfe.unsqueeze(0), dfe.unsqueeze(0), dfe.unsqueeze(0)}, 0);
  out.writeln("treatment rate r2        ", std::vector<double>{0.0, 0.2, 0.5, 1.0, 2.0});
  out.writeln("R0                       ", flat(epi::reproduction_number(epi::treatment_split(rates), lanes5)));
}
```

Output

```text
beta1                    [0.5000, 1.0000, 1.5000, 2.0000]
R0                       [0.7958, 1.5915, 2.3873, 3.1830]
treatment rate r2        [0.0000, 0.2000, 0.5000, 1.0000, 2.0000]
R0                       [16.3636, 3.7657, 1.7476, 0.9231, 0.4749]
```

$R_0$ is proportional to the infection rate: `0.7958`, `1.5915`, `2.3873`, `3.1830` for $\beta_1 = 0.5, 1, 1.5, 2$, so the threshold $R_0 = 1$ lies at $\beta_1 \approx 0.63$. It falls steeply with the treatment rate of infectious people: from `16.3636` with no treatment of infectious people through `3.7657` and `1.7476` to `0.9231` at $r_2 = 1$, so that treating infectious people at about one per time unit brings the epidemic under the threshold, and `0.4749` at $r_2 = 2$.

## Decisions

- Use `reproduction_number` for a threshold, and `next_generation` when you want to know who infects whom: the entries show which compartment drives the spread.
- Evaluate at the disease-free state only. For a model that is not one of the supplied ones, write the two functions `infections` and `transfers` of the full state and set `infected`.
- Pass tables of parameters on a leading axis, one disease-free state per row, rather than looping.
- Treat $R_0 = 1$ as a threshold, not as a prediction of size: the size of an epidemic needs the full model or the final-size relation of [Biology 6](../bio_06_epidemics.md).

## Pitfalls

The split is checked for consistency before the matrix is trusted. The program gives a split whose new infections fall as the infected grow, one whose number of infected compartments exceeds the state, and finally a state that is not disease free.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace epi = biology::epidemiology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an F/V split that is not epidemiologically consistent is rejected: V must have non-positive off-diagonals and F must be non-negative
  epi::Split bad;
  bad.infected = 1;
  bad.infections = [](const pluss::Series &x) { return x.neg(); };   // new infections that fall as the infected grow
  bad.transfers = [](const pluss::Series &x) { return x; };
  try {
    epi::next_generation(bad, vec({0.0, 0.0}));
  } catch (const pluss::Error &e) {
    out.writeln("negative infections: ", e.message());
  }
  // asking for more infected compartments than the state has
  epi::Split wide = epi::treatment_split(epi::TreatmentParams{});
  wide.infected = 9;
  try {
    epi::next_generation(wide, vec({0.0, 0.0, 1.0, 0.0}));
  } catch (const pluss::Error &e) {
    out.writeln("too many infected compartments: ", e.message());
  }
  // the matrix is only meaningful at the disease-free state; elsewhere the sign checks may refuse it
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  out.writeln("R0 at the disease-free state  ", flat(epi::reproduction_number(epi::treatment_split(p), vec({0.0, 0.0, 1.0, 0.0}))));
  try {
    out.writeln("R0 at a state with infection present ", flat(epi::reproduction_number(epi::treatment_split(p), vec({0.1, 0.1, 0.7, 0.1}))));
  } catch (const pluss::Error &e) {
    out.writeln("state with infection present: ", e.message());
  }
}
```

Output

```text
negative infections: natsci: epidemiology: F must be non-negative at the DFE
too many infected compartments: natsci: epidemiology: m exceeds compartments
R0 at the disease-free state  [2.3873]
state with infection present: natsci: epidemiology: F must be non-negative at the DFE
```

The first is refused because $F$ must be non-negative: new infections cannot be a decreasing function of the infected, and a minus sign in a hand-written split is the usual cause. The second is refused because the state has fewer compartments than the split claims. At the proper disease-free state $R_0 = 2.3873$; at a state with infection present the matrix is refused: the construction is only valid at the disease-free equilibrium.

## Where next

Continue with [Stages, strains and vectors](structured_models.md) for the other supplied models, and see [Biology 6: Epidemics](../bio_06_epidemics.md) for the SIR and SEIR dynamics behind these thresholds.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="structured_models.html">
    <span class="label">Next</span>
    <span class="title">Stages, strains and vectors</span>
  </a>
</div>
<!-- apiref:nav:end -->
