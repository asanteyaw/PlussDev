# Drift and fixation

[Back to Biology 8: Genetics](../bio_08_genetics.md)

In a finite population some alleles are lost by chance even if neutral, and a new advantageous mutant is often lost before it can spread. This page uses the Moran process, in which one individual reproduces and one dies at each step, to find the chance that a new mutant takes over and the time it takes, and compares that with random drift in simulated populations.

## How it works

`moran_transition(n, r)` returns the exact transition matrix of the number of mutants in a population of `n`, where a mutant has relative fitness `r`, and `absorption` of that matrix returns, for every number of mutants, the probability `fixation` that mutants eventually take over and the expected `time` until one type is left. `moran_fixation(r, n, start)` is the closed form of the same probability. `genetics::simulate(p0, population, w, max_generations, seed)` is the Wright-Fisher model: each generation draws the `2N` gametes at random from the expected frequency, so every lane of `p0` is an independent population.

## Chance of takeover

The program compares the exact chain with the closed form for three relative fitnesses in a population of `50`, prints takeover chances and mean times from several starting numbers, and then simulates `200` neutral populations of `100` starting at `50%`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace genetics = biology::genetics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a population of N = 50 in which a mutant with relative fitness r replaces others one at a time (Moran process)
  const std::int64_t n = 50;
  const auto start = vec({1.0, 10.0, 25.0});
  for (double r : {1.0, 1.1, 1.5}) {
    const auto exact = genetics::absorption(genetics::moran_transition(n, r));
    out.writeln("r = ", r, ":  chance a lone mutant takes over = ", flat(exact.fixation)[1],
                ", closed form = ", flat(genetics::moran_fixation(r, n, vec({1.0})))[0]);
  }
  const auto chain = genetics::absorption(genetics::moran_transition(n, 1.1));
  out.writeln("r = 1.1: takeover chance from 1, 10, 25 mutants = ", flat(genetics::moran_fixation(1.1, n, start)));
  out.writeln("r = 1.1: expected steps until one type is left, from 1, 10, 25 mutants = ",
              std::vector<double>{flat(chain.time)[1], flat(chain.time)[10], flat(chain.time)[25]});

  // finite populations with random drift: Wright-Fisher, 200 replicate populations of 100 individuals
  const auto p0 = vec(std::vector<double>(200, 0.5));
  const auto run = genetics::simulate(p0, 100, genetics::additive(0.0), 2000, 7);
  const auto f = flat(run.frequency);
  int fixed_a = 0;
  for (double x : f) fixed_a += (x >= 1.0) ? 1 : 0;
  out.writeln("neutral drift from 50%: ", fixed_a, " of 200 populations ended with A fixed, after ", run.generations, " generations, all absorbed: ", run.absorbed ? "yes" : "no");
}
```

Output

```text
r = 1.0000:  chance a lone mutant takes over = 0.0200, closed form = 0.0200
r = 1.1000:  chance a lone mutant takes over = 0.0917, closed form = 0.0917
r = 1.5000:  chance a lone mutant takes over = 0.3333, closed form = 0.3333
r = 1.1: takeover chance from 1, 10, 25 mutants = [0.0917, 0.6197, 0.9155]
r = 1.1: expected steps until one type is left, from 1, 10, 25 mutants = [296.7290, 1311.9583, 1260.4430]
neutral drift from 50%: 94 of 200 populations ended with A fixed, after 1296 generations, all absorbed: yes
```

A neutral mutant (`r = 1`) takes over with probability `1/50 = 0.02`; a mutant that is `10%` fitter with probability only `0.0917`, and one that is `50%` fitter with `0.3333`: even a large advantage is often lost to chance when the mutant is rare. The exact chain and the closed form agree to four digits. Starting from `25` mutants the takeover chance is `0.9155`. In the neutral simulation `94` of `200` populations ended with `A` fixed, about the expected half, and every one was absorbed in the end.

<!-- addex:start -->
## Population size and drift

Drift acts faster in small populations. `genetics::simulate` runs every starting frequency as its own population, so the program starts `300` neutral populations at {math}`50\%` for each of three population sizes and reports when every one has lost an allele and how many ended with {math}`A`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace genetics = biology::genetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // drift is stronger in a small population: 300 neutral replicate populations of 10, 50 and 250 individuals, all starting at 50%
  for (std::int64_t n : {10, 50, 250}) {
    const auto p0 = vec(std::vector<double>(300, 0.5));
    const auto run = genetics::simulate(p0, n, genetics::additive(0.0), 6000, 11);
    int fixed = 0; for (double x : flat(run.frequency)) fixed += (x >= 1.0) ? 1 : 0;
    out.writeln("population ", n, ": all 300 populations had lost an allele after ", run.generations, " generations (", run.absorbed ? "yes" : "no", "); A won in ", fixed);
  }
}
```

Output

```text
population 10: all 300 populations had lost an allele after 120 generations (yes); A won in 143
population 50: all 300 populations had lost an allele after 592 generations (yes); A won in 147
population 250: all 300 populations had lost an allele after 5000 generations (yes); A won in 141
```

In a population of `10` all `300` replicates were absorbed within `120` generations, for `50` within `592` and for `250` only by generation `5000`: the time to lose an allele grows with the population size. In every case {math}`A` wins about half of the time (`143`, `147` and `141` of `300`) since the alleles are neutral and symmetric.

## Selection against drift

A new advantage is not enough to guarantee success. The program starts {math}`A` at {math}`10\%` in `500` populations of `100` individuals with three selection strengths and counts how often it is fixed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace genetics = biology::genetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // selection against drift: a population of 100 where A starts at 10% with a 5% advantage per copy, 500 replicates, against the neutral case
  for (double s : {0.0, 0.05, 0.2}) {
    const auto p0 = vec(std::vector<double>(500, 0.1));
    const auto run = genetics::simulate(p0, 100, genetics::additive(s), 4000, 5);
    int fixed = 0; for (double x : flat(run.frequency)) fixed += (x >= 1.0) ? 1 : 0;
    out.writeln("s = ", s, ": A fixed in ", fixed, " of 500 populations (", fixed / 500.0, ")");
  }
}
```

Output

```text
s = 0.0000: A fixed in 47 of 500 populations (0.0940)
s = 0.0500: A fixed in 422 of 500 populations (0.8440)
s = 0.2000: A fixed in 499 of 500 populations (0.9980)
```

With no selection {math}`A` is fixed in `47` of `500` populations (`0.0940`), close to its starting frequency of `0.1`. With a {math}`5\%` advantage it is fixed in `422` (`0.8440`) and with a {math}`20\%` advantage in `499`. Even a strong advantage is occasionally lost, but the weaker the advantage the more the outcome is left to chance.

## How the Moran process scales

The exact Moran chain gives the takeover chance and the time to absorption for any population size. The program takes a lone mutant with a {math}`10\%` fitness advantage in populations of `10` to `80`, and compares both numbers with the neutral values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace genetics = biology::genetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the Moran process at several population sizes: takeover chance of a lone mutant with r = 1.1, and the steps until one type is left
  for (std::int64_t n : {10, 20, 40, 80}) {
    const auto chain = genetics::absorption(genetics::moran_transition(n, 1.1));
    const auto neutral = genetics::absorption(genetics::moran_transition(n, 1.0));
    out.writeln("N = ", n, ": takeover chance ", flat(chain.fixation)[1], " (neutral ", 1.0 / n, "), steps to absorption ", flat(chain.time)[1], " (neutral ", flat(neutral.time)[1], ")");
  }
}
```

Output

```text
N = 10: takeover chance 0.1480 (neutral 0.1000), steps to absorption 30.5825 (neutral 28.2897)
N = 20: takeover chance 0.1068 (neutral 0.0500), steps to absorption 82.7453 (neutral 70.9548)
N = 40: takeover chance 0.0930 (neutral 0.0250), steps to absorption 218.6796 (neutral 170.1417)
N = 80: takeover chance 0.0910 (neutral 0.0125), steps to absorption 553.8623 (neutral 396.2383)
```

The takeover chance falls with the population size, from `0.1480` for `10` to `0.0910` for `80`, but it falls much more slowly than the neutral value `1/N`: the advantage matters more the larger the population, `7.3` times the neutral chance at `N = 80`. The steps to absorption grow quickly with {math}`N` (`30.58` to `553.86`) and are longer than the neutral ones for every size.

<!-- addex:end -->

## Decisions

- Use `moran_transition` with `absorption` for an exact answer at moderate `n`; use `genetics::simulate` for large populations or when you want a spread of outcomes.
- Always read `run.absorbed`: it says whether every population reached `0` or `1` within the limit.

## Pitfalls

A simulation that stops early leaves most populations between the extremes, and counting the fixed ones then undercounts. The program runs the page's `200` neutral populations with a limit of `50` and of `2000` generations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace genetics = biology::genetics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a simulation that stops before the populations are absorbed leaves frequencies strictly between 0 and 1
  const auto p0 = vec(std::vector<double>(200, 0.5));
  for (int limit : {50, 2000}) {
    const auto run = genetics::simulate(p0, 100, genetics::additive(0.0), limit, 7);
    int fixed = 0, lost = 0, open = 0;
    for (double x : flat(run.frequency)) (x >= 1.0 ? fixed : x <= 0.0 ? lost : open)++;
    out.writeln("limit ", limit, " generations: fixed ", fixed, ", lost ", lost, ", still polymorphic ", open, ", absorbed: ", run.absorbed ? "yes" : "no");
  }
}
```

Output

```text
limit 50 generations: fixed 2, lost 2, still polymorphic 196, absorbed: no
limit 2000 generations: fixed 94, lost 106, still polymorphic 0, absorbed: yes
```

After `50` generations only `4` of `200` have been absorbed (`2` fixed, `2` lost) and `196` are still polymorphic, with `absorbed: no`. After `2000` generations all are absorbed, `94` fixed and `106` lost. A fixation fraction is meaningful only when `absorbed` is `yes`.

## Where next

[Selection](selection.md) adds a fitness difference to the same populations, and [Biology 14: Gene Circuits](../bio_14_gene_circuits.md) follows the noise inside a single cell.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="selection.html">
    <span class="label">Previous</span>
    <span class="title">Selection</span>
  </a>
</div>
<!-- apiref:nav:end -->
