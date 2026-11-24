# Selection

[Back to Biology 8: Genetics](../bio_08_genetics.md)

When one genotype leaves more offspring than another, its allele spreads. This page follows the frequency of a favoured allele generation by generation under selection, and shows how an advantage for the heterozygote keeps both alleles in the population.

## How it works

A `genetics::Fitness` gives the relative fitness of the genotypes `AA`, `Aa` and `aa`; `additive(s)` is the common case in which each copy of `A` adds `s`, giving `1 + 2s`, `1 + s` and `1`. `expected_next(p, w)` returns the frequency of `A` in the next generation from the frequency `p` now, `mean_fitness(p, w)` the average fitness of the population, and `hardy_weinberg(p)` the genotype shares `(AA, Aa, aa)` that random mating produces. For fitnesses `1 - s`, `1`, `1 - t` (the heterozygote best), `heterozygote_advantage_equilibrium(s, t)` is the stable frequency `t / (s + t)`. All of them work on arrays of frequencies at once.

## A favoured allele

Allele `A` is favoured with `s = 0.1`. The program starts three populations at `1%`, `5%` and `20%`, steps them together for 100 generations and prints the frequencies at three generations, then the mean fitness, the genotype shares at `30%` and the equilibrium of a population in which the heterozygote is fittest.

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
  // allele A starts at 1% and is favoured: AA 1.2, Aa 1.1, aa 1.0 (additive, s = 0.1)
  const auto w = genetics::additive(0.1);
  auto p = vec({0.01, 0.01, 0.01}).mul(vec({1.0, 5.0, 20.0}));  // three starting frequencies, 1%, 5%, 20%
  out.writeln("generation 0   ", flat(p));
  for (int g = 1; g <= 100; ++g) {
    p = genetics::expected_next(p, w);
    if (g == 25 || g == 50 || g == 100) out.writeln("generation ", g, "   ", flat(p));
  }
  out.writeln("mean fitness at 50% A = ", flat(genetics::mean_fitness(vec({0.5}), w)));
  out.writeln("genotype shares at 30% A (AA, Aa, aa) = ", flat(genetics::hardy_weinberg(vec({0.3}))));

  // heterozygote advantage: AA 0.9, Aa 1.0, aa 0.7 keeps both alleles
  out.writeln("polymorphic equilibrium frequency of A = ", flat(genetics::heterozygote_advantage_equilibrium(vec({0.1}), vec({0.3}))));
}
```

Output

```text
generation 0   [0.0100, 0.0500, 0.2000]
generation 25   [0.0979, 0.3550, 0.7113]
generation 50   [0.5256, 0.8395, 0.9570]
generation 100   [0.9891, 0.9976, 0.9994]
mean fitness at 50% A = [1.1000]
genotype shares at 30% A (AA, Aa, aa) = [0.0900, 0.4200, 0.4900]
polymorphic equilibrium frequency of A = [0.7500]
```

A rare favoured allele spreads slowly at first and then quickly: from `1%` it is at `9.79%` after 25 generations and at `52.56%` after 50, and by generation 100 all three populations are above `98%`. At `50%` the mean fitness is `1.1000`, midway between the homozygotes. At `30%` the genotype shares are `0.09`, `0.42` and `0.49`. When the heterozygote is fittest, selection against both homozygotes stops at an interior point, here `0.75`, so the allele is neither lost nor fixed.

<!-- addex:start -->
## Dominance changes the pace

How an advantage is expressed matters as much as its size. A `genetics::Fitness` holds the fitness of each genotype, `aa`, `ab` and `bb` for {math}`AA`, {math}`Aa` and {math}`aa`, and any of them may be set separately. The program gives allele {math}`A` the same top fitness of {math}`1.1` in three ways, additive, dominant (both {math}`AA` and {math}`Aa` benefit) and recessive (only {math}`AA` benefits), and follows the frequency from {math}`0.1`.

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
  // the same advantage for allele A, expressed in three ways (genotype fitnesses AA, Aa, aa)
  genetics::Fitness additive = genetics::additive(0.1);          // 1.2, 1.1, 1.0
  genetics::Fitness dominant;  dominant.aa = 1.1;  dominant.ab = 1.1;  dominant.bb = 1.0;
  genetics::Fitness recessive; recessive.aa = 1.1; recessive.ab = 1.0;  recessive.bb = 1.0;
  const auto run = [&](const char *label, const genetics::Fitness &w) {
    auto p = vec({0.1});
    std::vector<double> at;
    for (int g = 1; g <= 150; ++g) {
      p = genetics::expected_next(p, w);
      if (g == 25 || g == 50 || g == 100 || g == 150) at.push_back(flat(p)[0]);
    }
    out.writeln(label, at);
  };
  out.writeln("frequency of A at generations 25, 50, 100, 150, starting from 0.1");
  run("additive          ", additive);
  run("A dominant        ", dominant);
  run("A recessive       ", recessive);
}
```

Output

```text
frequency of A at generations 25, 50, 100, 150, starting from 0.1
additive          [0.5313, 0.9129, 0.9988, 1.0000]
A dominant        [0.4040, 0.6625, 0.8474, 0.9056]
A recessive       [0.1281, 0.1752, 0.4557, 0.9691]
```

The additive advantage reaches `0.5313` after `25` generations and fixation by `150`. The dominant advantage acts at once on the heterozygotes, so it is fast at first (`0.4040`) but slows as {math}`A` becomes common, because the remaining {math}`aa` individuals are few (`0.9056` at generation `150`). The recessive advantage is invisible while {math}`A` is rare, since few {math}`AA` exist: it moves only to `0.1281` in `25` generations, then accelerates to `0.9691` by generation `150`.

## Mutation and selection in balance

Selection removes a harmful allele, and mutation makes it again, so the frequency settles where the two balance. `expected_next(p, w, mu, nu)` includes forward and back mutation at rates `mu` and `nu`. The program lets {math}`aa` have fitness {math}`0.5` and mutation {math}`A \to a` occur at {math}`10^{-3}`, and follows the frequency of the harmful allele from zero.

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
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // a recessive harmful allele a is removed by selection (aa has fitness 0.5) and made again by mutation A -> a at rate 1e-3
  genetics::Fitness w; w.aa = 1.0; w.ab = 1.0; w.bb = 0.5;
  auto p = vec({1.0});      // start with only the normal allele
  for (int g = 1; g <= 600; ++g) {
    p = genetics::expected_next(p, w, 1.0e-3, 0.0);
    if (g == 10 || g == 50 || g == 200 || g == 600) out.writeln("generation ", g, ": frequency of the harmful allele = ", 1.0 - flat(p)[0]);
  }
  out.writeln("balance predicted by sqrt(mu / s) = ", std::sqrt(1.0e-3 / 0.5));
}
```

Output

```text
generation 10: frequency of the harmful allele = 0.00982
generation 50: frequency of the harmful allele = 0.03590
generation 200: frequency of the harmful allele = 0.04471
generation 600: frequency of the harmful allele = 0.04472
balance predicted by sqrt(mu / s) = 0.04472
```

The harmful allele climbs from zero, `0.00982` at generation `10` and `0.03590` at `50`, and levels off at `0.04472`, which is {math}`\sqrt{\mu/s} = \sqrt{10^{-3}/0.5}` to the digits printed.

## A stable polymorphism

When the heterozygote is the fittest, both alleles are kept. The program runs three environments with different selection against each homozygote in one batch, started from a rare and from a common allele, and compares the end point with the equilibrium {math}`t/(s + t)` and prints the mean fitness there.

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
  // heterozygote advantage: AA 1 - s, Aa 1, aa 1 - t. Three environments in one batch, started from rare and from common A
  const auto s = vec({0.1, 0.2, 0.3}), t = vec({0.3, 0.2, 0.1});
  genetics::Fitness w; w.aa = s.neg().add(1.0); w.ab = 1.0; w.bb = t.neg().add(1.0);
  auto low = vec({0.05, 0.05, 0.05}), high = vec({0.95, 0.95, 0.95});
  for (int g = 0; g < 300; ++g) { low = genetics::expected_next(low, w); high = genetics::expected_next(high, w); }
  out.writeln("predicted equilibrium t/(s + t) = ", flat(genetics::heterozygote_advantage_equilibrium(s, t)));
  out.writeln("from a rare start after 300 generations   = ", flat(low));
  out.writeln("from a common start after 300 generations = ", flat(high));
  out.writeln("mean fitness at the equilibrium = ", flat(genetics::mean_fitness(low, w)));
}
```

Output

```text
predicted equilibrium t/(s + t) = [0.7500, 0.5000, 0.2500]
from a rare start after 300 generations   = [0.7500, 0.5000, 0.2500]
from a common start after 300 generations = [0.7500, 0.5000, 0.2500]
mean fitness at the equilibrium = [0.9250, 0.9000, 0.9250]
```

From both starting points all three environments end at the predicted `0.7500`, `0.5000` and `0.2500`. The mean fitness at the equilibrium, `0.9250`, `0.9000` and `0.9250`, is below `1`, which is the cost of keeping the less fit homozygotes in the population.

<!-- addex:end -->

## Decisions

- Use `expected_next` for an infinite population and `genetics::simulate` for a finite one.
- Use `additive`, or set the three fitnesses of a `Fitness` yourself, to match the dominance you have measured.
- Remember that every frequency here is the frequency of allele `A`.

## Pitfalls

Two conventions cause most errors: the order of the arguments of `heterozygote_advantage_equilibrium(s, t)`, which are the losses of `AA` and `aa`, and the fact that the frequency is that of `A`. The program swaps the arguments, and then steps a favoured allele from `1%` and from `99%`.

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
  // heterozygote_advantage_equilibrium(s, t) is for fitnesses 1 - s, 1, 1 - t: the order of the two arguments matters
  out.writeln("AA 0.9, Aa 1.0, aa 0.7 (s = 0.1, t = 0.3): ", flat(genetics::heterozygote_advantage_equilibrium(vec({0.1}), vec({0.3}))));
  out.writeln("arguments swapped:                          ", flat(genetics::heterozygote_advantage_equilibrium(vec({0.3}), vec({0.1}))));
  // the frequency is for allele A: starting a favoured allele at 99% instead of 1% reverses the story
  auto p = vec({0.01, 0.99});
  for (int g = 0; g < 20; ++g) p = genetics::expected_next(p, genetics::additive(0.1));
  out.writeln("after 20 generations from 1% and 99%: ", flat(p));
}
```

Output

```text
AA 0.9, Aa 1.0, aa 0.7 (s = 0.1, t = 0.3): [0.7500]
arguments swapped:                          [0.2500]
after 20 generations from 1% and 99%: [0.0633, 0.9982]
```

For fitnesses `0.9`, `1.0` and `0.7` the equilibrium is `0.7500`; with the arguments swapped it is `0.2500`, the frequency of the other allele. In the second line the favoured allele from `1%` has reached only `0.0633` after `20` generations while from `99%` it is at `0.9982`: where you start matters as much as the advantage.

## Where next

[Drift and fixation](drift.md) explains why a rare favoured allele can still be lost, and [Biology 26: Adaptive Dynamics](../bio_26_adaptation.md) lets the trait itself evolve.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="drift.html">
    <span class="label">Next</span>
    <span class="title">Drift and fixation</span>
  </a>
</div>
<!-- apiref:nav:end -->
