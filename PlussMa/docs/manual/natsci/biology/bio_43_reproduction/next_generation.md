# The next-generation method

[Back to Biology 43: Reproduction Numbers](../bio_43_reproduction.md)

An infection spreads when each case causes more than one new case, and you want that threshold, the reproduction number $R_0$, for a model that has more than one infected compartment: a latent stage, a treatment programme, a mosquito vector, several strains, or an infection that passes through stages. This page computes $R_0$ from any such model with the next-generation matrix, shows how treatment and strain competition change it, and checks the threshold by simulation.

## How it works

The code is in `pluss::ma::natsci::biology::epidemiology` (`epi::` below). Split the equations for the infected compartments into $\dot x = F(x) - V(x)$, where $F$ holds the appearance of new infections and $V$ everything else (progression, recovery, death, treatment). At the disease-free state $x_0$, take the Jacobians $F = \partial F/\partial x$ and $V = \partial V/\partial x$ restricted to the infected compartments. The next-generation matrix is $K = F V^{-1}$: its entry $(i, j)$ is the expected number of new cases in compartment $i$ caused by one case in compartment $j$ over its whole infectious life. The reproduction number is its largest eigenvalue,

```{math}
R_0 = \rho\!\left(F V^{-1}\right)
```

and the disease dies out when $R_0 < 1$ and can invade when $R_0 > 1$. A `Split` holds the two functions and the number of infected compartments. The library builds one for each model: `epi::treatment_split(p)` with `epi::treatment_rate`, `epi::vector_host_split(p)` with `epi::vector_host_rate`, `epi::multistrain_split(p)` with `epi::multistrain_rate`, and `epi::staged_split(beta, nu, d, b)` with `epi::staged_rate`. Then `epi::next_generation(split, dfe)` returns $K$ and `epi::reproduction_number(split, dfe)` returns $R_0$, both evaluated at the disease-free state `dfe`, listed in the compartment order of the model. Parameters may carry leading batch axes, so a sweep is one call.

## Treatment

The treatment model has compartments (latent $E$, infectious $I$, susceptible $S$, treated $T$). The program prints the next-generation matrix and $R_0$ with and without treatment, then sweeps the treatment rate of the infectious class $r_2$ over six values in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace epi = biology::epidemiology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // tuberculosis-like model, compartments (E latent, I infectious, S susceptible, T treated); the disease-free state is (0, 0, 1, 0)
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  const auto dfe = vec({0.0, 0.0, 1.0, 0.0});
  const auto split = epi::treatment_split(p);
  out.writeln("next-generation matrix (infected block E, I) = ", flat(epi::next_generation(split, dfe)));
  out.writeln("R0 with treatment    = ", flat(epi::reproduction_number(split, dfe))[0]);
  epi::TreatmentParams none = p;
  none.r1 = 0.0; none.r2 = 0.0;
  out.writeln("R0 without treatment = ", flat(epi::reproduction_number(epi::treatment_split(none), dfe))[0]);
  // how much treatment is enough? sweep the treatment rate of the infectious class, one call for the whole sweep
  const auto r2 = vec({0.0, 0.35, 0.7, 1.4, 2.8, 5.6});
  epi::TreatmentParams sweep = p;
  sweep.r2 = r2.unsqueeze(-1);
  auto lanes = dfe.unsqueeze(0);
  for (int i = 1; i < 6; ++i) lanes = lanes.unite(std::vector<pluss::Series>{dfe.unsqueeze(0)}, 0);
  out.writeln("treatment rate r2 = ", flat(r2));
  out.writeln("R0                = ", flat(epi::reproduction_number(epi::treatment_split(sweep), lanes)));
}
```

Output

```text
next-generation matrix (infected block E, I) = [2.3873, 4.3767, 0.0000, 0.0000]
R0 with treatment    = 2.3873
R0 without treatment = 25.7143
treatment rate r2 = [0.0000, 0.3500, 0.7000, 1.4000, 2.8000, 5.6000]
R0                = [16.3636, 2.3873, 1.2876, 0.6701, 0.3421, 0.1728]
```

Without treatment $R_0 = \beta_1\nu/(d(d+\nu)) = 25.7143$; the treatment programme brings it to `2.3873`. Only the first row of the matrix is non-zero, `2.3873` and `4.3767`, because in this model new infections appear in the latent class only. In the sweep $R_0$ falls from `16.3636` with no treatment of the infectious to `1.2876` at $r_2 = 0.7$ and below one, `0.6701`, at $r_2 = 1.4$: that is the treatment rate you need to stop the epidemic.

## Vectors

In a vector-borne disease the infection passes from host to vector and back, so one generation is two transmissions. The program computes $R_0$ for a malaria-like model, compares it with the closed formula, and simulates a small outbreak with a weak and with a strong transmission from host to vector.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace epi = biology::epidemiology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // malaria-like transmission: state (infected hosts I, infected vectors V, susceptible hosts S, susceptible vectors M), all as fractions
  epi::VectorHostParams p;
  p.beta_s = 0.8; p.beta_m = 0.6; p.b = 0.1; p.c = 0.4; p.gamma = 0.2;
  const auto dfe = vec({0.0, 0.0, 1.0, 1.0});
  out.writeln("R0 = ", flat(epi::reproduction_number(epi::vector_host_split(p), dfe))[0], "  (sqrt(beta_s beta_m / ((b + gamma) c)) = ", std::sqrt(0.8 * 0.6 / (0.3 * 0.4)), ")");
  // below and above the threshold, from a small outbreak
  for (double beta_m : {0.1, 0.6}) {
    epi::VectorHostParams q = p;
    q.beta_m = beta_m;
    const auto rate = [&](const pluss::Series &x) { return epi::vector_host_rate(x, q); };
    out.writeln("beta_m = ", beta_m, ": R0 = ", flat(epi::reproduction_number(epi::vector_host_split(q), dfe))[0], ", state after 400 days (I, V, S, M) = ",
                flat(dynamics::simulate(rate, vec({0.01, 0.0, 0.99, 1.0}), 0.0, 400.0, 8000)));
  }
}
```

Output

```text
R0 = 2.0000  (sqrt(beta_s beta_m / ((b + gamma) c)) = 2.0000)
beta_m = 0.1000: R0 = 0.8165, state after 400 days (I, V, S, M) = [0.0000, 0.0000, 1.0000, 1.0000]
beta_m = 0.6000: R0 = 2.0000, state after 400 days (I, V, S, M) = [0.5455, 0.4500, 0.4545, 0.5500]
```

$R_0 = 2.0000$ agrees with $\sqrt{\beta_s\beta_m/((b+\gamma)c)}$: it is the geometric mean of the two transmission steps. With $\beta_m = 0.1$, $R_0 = 0.8165 < 1$ and a small outbreak vanishes (`0.0000` infected after `400` days). With $\beta_m = 0.6$, $R_0 = 2$ and the infection settles at an endemic level, `0.5455` of hosts and `0.4500` of vectors infected.

## Competing strains

Two strains with $R_0 = 1.8$ and $2.0$ compete for the same hosts. The program computes the $R_0$ of the pair and follows a mixed outbreak.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace epi = biology::epidemiology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two strains compete for the same hosts; state (I1, I2, S). Strain 1 has R0 = 0.9/0.5 = 1.8 and strain 2 has R0 = 1.4/0.7 = 2.0
  epi::MultistrainParams p;
  p.beta1 = 0.9; p.beta2 = 1.4; p.b = 0.2; p.gamma1 = 0.3; p.gamma2 = 0.5; p.nu = 0.0;
  out.writeln("R0 of the pair = ", flat(epi::reproduction_number(epi::multistrain_split(p), vec({0.0, 0.0, 1.0})))[0], " (the larger of the two strain values)");
  const auto rate = [&](const pluss::Series &x) { return epi::multistrain_rate(x, p); };
  for (double t : {50.0, 200.0, 1000.0, 4000.0})
    out.writeln("time ", t, ": (I1, I2, S) = ", flat(dynamics::simulate(rate, vec({0.05, 0.01, 0.94}), 0.0, t, 40000)));
}
```

Output

```text
R0 of the pair = 2.0000 (the larger of the two strain values)
time 50.0000: (I1, I2, S) = [0.0473, 0.4488, 0.5038]
time 200.0000: (I1, I2, S) = [0.0000, 0.5000, 0.5000]
time 1000.0000: (I1, I2, S) = [0.0000, 0.5000, 0.5000]
time 4000.0000: (I1, I2, S) = [0.0000, 0.5000, 0.5000]
```

The $R_0$ of the pair is `2.0000`, the larger of the two. At time `50` both strains circulate (`0.0473` and `0.4488`), but strain 1 is shrinking: by time `200` it has gone and strain 2 holds the susceptible fraction at $1/R_0 = 0.5$. With no coinfection the strain with the larger $R_0$ excludes the other.

## Infection with stages

An infection can be more infectious early than late. The program computes $R_0$ for three infectious stages and the contribution of each stage when the others are switched off.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace epi = biology::epidemiology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an infection with three infectious stages that progress at rates nu and are removed at rates d; state (I1, I2, I3, S) with births b
  const auto beta = vec({0.5, 0.3, 0.2});         // infectiousness of the first m - 1 = 3 stages (m = 4: the last stage does not transmit)
  const auto nu = vec({0.4, 0.3, 0.2});           // progression rates between stages
  const auto d = vec({0.1, 0.15, 0.2, 0.3});      // removal from each of the four stages
  const auto split = epi::staged_split(beta, nu, d, 0.1);
  const auto dfe = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  out.writeln("R0 = ", flat(epi::reproduction_number(split, dfe))[0]);
  // the contribution of each stage to R0: switch the others off
  for (int stage = 0; stage < 3; ++stage) {
    std::vector<double> only(3, 0.0);
    only[stage] = flat(beta)[stage];
    out.writeln("stage ", stage + 1, " alone: R0 = ", flat(epi::reproduction_number(epi::staged_split(vec(only), nu, d, 0.1), dfe))[0]);
  }
}
```

Output

```text
R0 = 1.8000
stage 1 alone: R0 = 1.0000
stage 2 alone: R0 = 0.5333
stage 3 alone: R0 = 0.2667
```

$R_0 = 1.8000$. Switching the other stages off gives `1.0000`, `0.5333` and `0.2667`, which sum to `1.8`: $R_0$ is a sum over stages of (infectiousness) $\times$ (probability of reaching the stage) $\times$ (time spent there), and the first stage, though shortest, supplies more than half of it.

## Decisions

- Use `reproduction_number` when you only need the threshold and `next_generation` when you want to know who infects whom; the matrix shows which compartment drives the spread.
- Prefer the library splits to writing your own `Split`; if you do write one, put $F$ equal to new infections only, because a term wrongly placed in $V$ changes $R_0$.
- Sweep a parameter by giving it a leading batch axis and one lane of the disease-free state per value, as the treatment sweep does.

## Pitfalls

The disease-free state must be written in the compartment order of the model. The program evaluates the treatment model with the state in its own order, with the same numbers in another order, and at a state in the middle of an outbreak.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace dynamics = natsci::common::dynamics;
namespace epi = biology::epidemiology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the reproduction number is evaluated at the disease-free state, in the model's own compartment order
  epi::TreatmentParams p;
  p.beta1 = 1.5; p.beta2 = 0.4; p.d = 0.05; p.nu = 0.3; p.r1 = 0.2; p.r2 = 0.35; p.q = 0.7;
  const auto split = epi::treatment_split(p);
  out.writeln("disease-free state in model order (E, I, S, T) : R0 = ", flat(epi::reproduction_number(split, vec({0.0, 0.0, 1.0, 0.0})))[0]);
  out.writeln("the same state written as (S, E, I, T)         : R0 = ", flat(epi::reproduction_number(split, vec({1.0, 0.0, 0.0, 0.0})))[0]);
  try {
    out.writeln("a state in the middle of an outbreak           : R0 = ", flat(epi::reproduction_number(split, vec({0.05, 0.1, 0.8, 0.05})))[0]);
  } catch (const pluss::Error &e) { out.writeln("a state in the middle of an outbreak           : error: ", e.message()); }
}
```

Output

```text
disease-free state in model order (E, I, S, T) : R0 = 2.3873
the same state written as (S, E, I, T)         : R0 = 0.0000
a state in the middle of an outbreak           : error: natsci: epidemiology: F must be non-negative at the DFE
```

The wrong order gives `0.0000` with no complaint, because the infected compartments of that vector hold no infection and so the matrix is zero: the call cannot know which order you meant. A state that is not disease-free is sometimes caught, as here, where $F$ would be negative. In either case check that the model's rate function returns zero at the state you pass.

## Where next

[Biology 6: Epidemics](../bio_06_epidemics.md) introduces the SIR and SEIR models whose threshold is the simplest case of this one, and [Biology 25: Reaction Networks](../bio_25_networks.md) gives the general tools for simulating a model written as reactions.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
