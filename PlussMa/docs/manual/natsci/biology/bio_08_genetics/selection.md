<!-- generated from selection.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="drift.html">
    <span class="label">Next</span>
    <span class="title">Drift and fixation</span>
  </a>
</div>
<!-- apiref:nav:end -->
