# Stages, strains and vectors

[Back to Biology 30: Reproduction Numbers of Structured Epidemics](../bio_30_reproduction_numbers.md)

A real infection often passes through stages of declining infectiousness, comes in several strains that compete, or needs a vector such as a mosquito to travel between hosts. You want $R_0$ for each of these, and you want to see the dynamics that the threshold predicts. This page uses the staged, multi-strain and vector-host models, whose rates and splits the library supplies, and checks the numerical threshold against what each model implies by hand.

## How it works

In the **staged** model a person passes through $m$ infectious stages; stage $i$ infects at rate $\beta_i$ (the last stage is not infectious), progresses to stage $i+1$ at rate $\nu_i$ and leaves at death rate $d_i$. A person's expected contribution is the sum over stages of the infectiousness times the probability of reaching the stage times the time spent in it:

```{math}
R_0 = \sum_{i=1}^{m-1}\beta_i\,\frac{\prod_{k<i}\nu_k}{\prod_{k\le i}(\nu_k + d_k)}\,.
```

`epidemiology::staged_rate(x, beta, nu, d, b)` and `staged_split(beta, nu, d, b)` take the state $(I_1,\dots,I_m, S)$. In the **multi-strain** model two strains with transmission $\beta_j$ and clearance $\gamma_j$ compete for one susceptible pool with deaths $b$; strain $j$ alone has $R_0^{(j)} = \beta_j/(b+\gamma_j)$, and the overall threshold is the larger of the two. In the **vector-host** model infected hosts $I$ and infected vectors $V$ infect each other; $R_0$ is the square root of the product of the two transmission loops, one host-to-vector and one vector-to-host,

```{math}
R_0 = \sqrt{\frac{\beta_s\,\beta_m}{(b+\gamma)\,c}}
```

with the host death and recovery rates $b$ and $\gamma$ and the vector death rate $c$. All of the rates return the state derivative, to integrate with `dynamics::simulate`, and all of the splits go to `reproduction_number`.

## Stages of infection

The program defines a disease with four infectious stages (the last not infectious), computes $R_0$ from the split, and reconstructs it by hand from the stage formula.

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
  // a disease with a chain of infectious stages: the early stage is the most infectious. Stage i passes to stage i+1 at rate nu_i; deaths d_i
  const auto beta = vec({0.5, 0.3, 0.2}), nu = vec({0.4, 0.3, 0.2}), d = vec({0.1, 0.15, 0.2, 0.3});
  const auto split = epi::staged_split(beta, nu, d, 0.1);
  const auto dfe = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  out.writeln("infected compartments    ", std::vector<double>{static_cast<double>(split.infected)});
  const auto k = epi::next_generation(split, dfe);
  out.writeln("first row of F V^-1       ", flat(k.select(0, 0)));
  out.writeln("reproduction number       ", flat(epi::reproduction_number(split, dfe)));
  // by hand: infectiousness accumulated over the stages, each stage weighted by the chance of reaching it and the time spent in it
  const double s1 = 0.4 + 0.1, s2 = 0.3 + 0.15, s3 = 0.2 + 0.2;
  const double hand = 0.5 / s1 + 0.4 / s1 * 0.3 / s2 + 0.4 / s1 * 0.3 / s2 * 0.2 / s3;
  out.writeln("by hand                   ", std::vector<double>{hand});
}
```

Output

```text
infected compartments    [4.0000]
first row of F V^-1       [1.8000, 1.0000, 0.5000, 0.0000]
reproduction number       [1.8000]
by hand                   [1.8000]
```

The split has `4` infected compartments. The first row of the next-generation matrix, `(1.8, 1.0, 0.5, 0)`, gives the expected number of cases caused by a person who starts in stage 1, 2, 3 or 4: `1.8`, `1.0`, `0.5` and none, because infectiousness fades along the course of the disease and the last stage is not infectious. New infections always start in stage 1, so $R_0$ is the first entry, `1.8`. The hand calculation $0.5/0.5 + (0.4/0.5)(0.3/0.45) + (0.4/0.5)(0.3/0.45)(0.2/0.4)$, the three stage contributions $1 + 0.5333 + 0.2667$, gives the same `1.8000`.

## Two strains compete

The program computes the threshold of each strain by hand and from the model, then introduces both strains at a tiny level and follows the epidemic to `t = 1000`.

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
  // two strains: the first has beta1 = 0.9 and clears at gamma1 = 0.3, the second beta2 = 1.4 and gamma2 = 0.5; deaths b = 0.2. R0 of a strain = beta / (b + gamma)
  epi::MultistrainParams p;
  p.beta1 = 0.9; p.beta2 = 1.4; p.b = 0.2; p.gamma1 = 0.3; p.gamma2 = 0.5; p.nu = 0.0;
  const auto dfe = vec({0.0, 0.0, 1.0});
  out.writeln("R0 of the two strains by hand ", std::vector<double>{0.9 / 0.5, 1.4 / 0.7});
  out.writeln("R0 of the model               ", flat(epi::reproduction_number(epi::multistrain_split(p), dfe)));
  // both strains introduced at a tiny level: the one with the higher R0 takes over
  const auto rate = [&](const pluss::Series &x) { return epi::multistrain_rate(x, p); };
  for (double t : {50.0, 200.0, 1000.0})
    out.writeln("t = ", std::vector<double>{t}, "  (I1, I2, S) ", flat(dynamics::simulate(rate, vec({0.001, 0.001, 0.998}), 0.0, t, 20000)));
}
```

Output

```text
R0 of the two strains by hand [1.8000, 2.0000]
R0 of the model               [2.0000]
t = [50.0000]  (I1, I2, S) [0.0044, 0.4952, 0.5003]
t = [200.0000]  (I1, I2, S) [0.0000, 0.5000, 0.5000]
t = [1000.0000]  (I1, I2, S) [0.0000, 0.5000, 0.5000]
```

The strains have $R_0^{(j)} = 1.8$ and $2.0$ and the model's threshold is the larger, `2.0000`. Both strains spread at first, but the second has the higher threshold, and by `t = 200` the first has vanished (`0.0000`) while the second holds `0.5000` of the population with a susceptible fraction `0.5000` $= 1/R_0$. Competitive exclusion: the strain that drives the susceptibles lowest wins.

## A vector-borne disease

The program computes $R_0$ of the vector-host model from its split and from the closed form, introduces a few infected hosts, and follows the infection; then it weakens the biting rates to see the infection die out.

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
  // a vector-borne disease: infected hosts I and vectors V, susceptible hosts S and vectors M. R0 is the square root of the product of the two transmission loops
  epi::VectorHostParams p;
  p.beta_s = 0.8; p.beta_m = 0.6; p.b = 0.1; p.c = 0.4; p.gamma = 0.2;
  const auto dfe = vec({0.0, 0.0, 1.0, 1.0});
  out.writeln("R0 from the next-generation matrix ", flat(epi::reproduction_number(epi::vector_host_split(p), dfe)));
  out.writeln("closed form sqrt(bs bm / ((b + g) c)) ", std::vector<double>{std::sqrt(0.8 * 0.6 / ((0.1 + 0.2) * 0.4))});
  // introduce a few infected hosts and follow the epidemic
  const auto rate = [&](const pluss::Series &x) { return epi::vector_host_rate(x, p); };
  for (double t : {0.0, 20.0, 60.0, 200.0})
    out.writeln("t = ", std::vector<double>{t}, "  (I, V, S, M) ", flat(t == 0.0 ? vec({0.01, 0.0, 0.99, 1.0}) : dynamics::simulate(rate, vec({0.01, 0.0, 0.99, 1.0}), 0.0, t, 8000)));
  // below threshold: halve the vector biting rate and the infection dies out
  epi::VectorHostParams weak = p;
  weak.beta_s = 0.2;
  weak.beta_m = 0.15;
  out.writeln("R0 with weaker biting      ", flat(epi::reproduction_number(epi::vector_host_split(weak), dfe)));
}
```

Output

```text
R0 from the next-generation matrix [2.0000]
closed form sqrt(bs bm / ((b + g) c)) [2.0000]
t = [0.0000]  (I, V, S, M) [0.0100, 0.0000, 0.9900, 1.0000]
t = [20.0000]  (I, V, S, M) [0.4955, 0.4072, 0.5045, 0.5928]
t = [60.0000]  (I, V, S, M) [0.5455, 0.4500, 0.4545, 0.5500]
t = [200.0000]  (I, V, S, M) [0.5455, 0.4500, 0.4545, 0.5500]
R0 with weaker biting      [0.5000]
```

The numerical and closed-form thresholds agree at `2.0000`. From one percent infected hosts the infection climbs to `0.4955` of hosts by time `20` and settles at an endemic state, `(0.5455, 0.4500, 0.4545, 0.5500)`, by time `60` that stays the same to time `200`. If the biting rates are cut to a quarter, $R_0$ falls to `0.5000` (it scales with the square root of the product of the two rates), and the infection cannot be sustained.

## Decisions

- Use the staged model when the infectiousness changes along the disease course, as in HIV or in a disease with an early peak.
- Use `multistrain` to ask which strain persists, and remember that its overall $R_0$ is a maximum: only the strain with the highest threshold survives in the end.
- Use `vector_host` when transmission needs a carrier; control the vector (lower $\beta_m$ or higher $c$) and check the threshold, which falls with the square root of the transmission loops.
- Check the numerical threshold against a closed form where you have one: it is a test of the parameters you typed.

## Pitfalls

The staged model needs one infectivity and one progression rate for every stage but the last, and a death rate for every stage. The program gives it the wrong lengths and then a single stage.

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
  // three infectious stages need two infectivities beta, two progression rates nu and three death rates d (the last stage is not infectious): wrong sizes are rejected
  try {
    epi::staged_rate(vec({0.0, 0.0, 0.0, 1.0}), vec({0.5, 0.3, 0.2}), vec({0.4, 0.3}), vec({0.1, 0.15, 0.2}), 0.1);
  } catch (const pluss::Error &e) {
    out.writeln("wrong rate lengths: ", e.message());
  }
  try {
    epi::staged_rate(vec({0.0, 1.0}), vec({}), vec({}), vec({0.1}), 0.1);
  } catch (const pluss::Error &e) {
    out.writeln("one stage: ", e.message());
  }
  // the correct sizes work: three stages, R0 from the next-generation matrix
  out.writeln("R0 with three stages ", flat(epi::reproduction_number(epi::staged_split(vec({0.5, 0.3}), vec({0.4, 0.3}), vec({0.1, 0.15, 0.2}), 0.1), vec({0.0, 0.0, 0.0, 1.0}))));
}
```

Output

```text
wrong rate lengths: natsci: staged: beta/nu need m-1 entries and d needs m
one stage: natsci: staged: need at least two infectious stages
R0 with three stages [1.5333]
```

The first is refused with the required lengths, `m-1` entries for the infectivity and progression and `m` for the death rates; the second because a staged model needs at least two stages. With the right lengths the model returns $R_0 = 1.5333$ for three stages, as the hand formula gives: $0.5/0.5 + (0.4/0.5)(0.3/0.45) = 1 + 0.5333$.

## Where next

Go back to [The next-generation matrix](next_generation.md) for the method, and to [Biology 6: Epidemics](../bio_06_epidemics.md) for the SIR and SEIR dynamics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="next_generation.html">
    <span class="label">Previous</span>
    <span class="title">The next-generation matrix</span>
  </a>
</div>
<!-- apiref:nav:end -->
