<!-- generated from quasi_random_sequences.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Quasi-random sequences

[Back to Monte Carlo](../finance_04_monte_carlo.md)

A pseudo-random sample of *N* points fills space irregularly, so the Monte Carlo error falls only as 1/√N. A *low-discrepancy* (quasi-random) sequence fills it evenly, and for smooth, low-dimensional problems the error can fall nearly as 1/N. `MonteCarloOptions::rand_method` (and `SimOptions::method`) selects the source of the shocks:

| `RandMethod` | Sequence |
|---|---|
| `Pseudo` | pseudo-random normals (the default) |
| `Sobol` | the Sobol sequence, with the standard direction numbers up to 1111 dimensions |
| `Halton` | the Halton sequence, a radical inverse in a different prime base for each dimension |
| `Torus` | the Kronecker sequence of square roots of primes |

## Paths are points, steps are dimensions

A path is one *point* of the sequence and each time step is one *dimension*; a Heston path with 32 steps uses 32 dimensions for its spot shocks and 32 more for the variance shocks, taken from separate blocks of one sequence, and a Bates path adds blocks for the jumps. A quasi-random sequence loses its advantage as the dimension grows, and the early dimensions of Sobol are better distributed than the late ones, so quasi-random sampling is most useful with few steps. The program shows the layout of the underlying generator, a Sobol sequence of `[points, dimensions]`, and the first points: dimension 1 is the van der Corput sequence 0.5, 0.75, 0.25, ... .

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rng = pluss::subsp::stats::rng;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  rng::SobolOptions options;
  options.init = true;
  options.start = 1;
  const auto points = rng::sobol({1024, 3}, options, as_f64());
  out.writeln("shape [points, dimensions]", points.sizes());
  const std::vector<double> first = points.select(1, 0).pull<double>();
  const std::vector<double> second = points.select(1, 1).pull<double>();
  out.writeln("first dimension, first eight points ", std::vector<double>(first.begin(), first.begin() + 8));
  out.writeln("second dimension, first eight points", std::vector<double>(second.begin(), second.begin() + 8));
  out.writeln("means of the three dimensions over 1024 points (0.5):", points.select(1, 0).mean().item<double>(), points.select(1, 1).mean().item<double>(), points.select(1, 2).mean().item<double>());
}
```

Output

```text
shape [points, dimensions] [1024, 3]
first dimension, first eight points  [0.5000, 0.7500, 0.2500, 0.3750, 0.8750, 0.6250, 0.1250, 0.1875]
second dimension, first eight points [0.5000, 0.2500, 0.7500, 0.3750, 0.8750, 0.1250, 0.6250, 0.3125]
means of the three dimensions over 1024 points (0.5): 0.4995 0.4999 0.5003
```

The generator returns an array of 1024 points in three dimensions. The first dimension begins 0.5, 0.75, 0.25, 0.375, 0.875, ... (the van der Corput sequence), the second is a different permutation of the same dyadic fractions, and each dimension has a mean of 0.5 to three decimals.

## Accuracy against pseudo-random numbers

A deterministic quasi-random set has no random error to measure, only an error against the exact answer, so it cannot report its own uncertainty. `randomized_quasi` randomises the sequence (a random digital shift, with `sobol_scrambling` choosing a scrambling of the digits, 0 to 3, for Sobol), so that repeating with different seeds yields independent unbiased estimates with an error bar. The program prices a Heston call with 16 time steps (a 32-dimensional problem) and compares the error of the deterministic sequences with the standard deviation of pseudo-random and randomised Sobol prices.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const vol::HestonParams params;
  const double reference = prc::heston_price_cos(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params);
  out.writeln("COS reference", reference);

  out.writeln("deterministic sequences, 16384 paths, 16 steps: error against the reference");
  for (auto [name, method] : {std::pair<const char *, vol::RandMethod>{"Pseudo ", vol::RandMethod::Pseudo}, {"Sobol  ", vol::RandMethod::Sobol}, {"Halton ", vol::RandMethod::Halton}, {"Torus  ", vol::RandMethod::Torus}}) {
    prc::MonteCarloOptions options;
    options.paths = 16384;
    options.steps = 16;
    options.rand_method = method;
    out.writeln("  ", name, prc::heston_price_mc(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params, options) - reference);
  }

  auto spread = [&](prc::MonteCarloOptions options) {
    std::vector<double> prices;
    for (int seed = 1; seed <= 16; ++seed) {
      options.seed = 37 * seed;
      prices.push_back(prc::heston_price_mc(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params, options));
    }
    double mean = 0.0;
    for (double p : prices) {
      mean += p / 16.0;
    }
    double variance = 0.0;
    for (double p : prices) {
      variance += (p - mean) * (p - mean) / 15.0;
    }
    return std::make_pair(mean - reference, std::sqrt(variance));
  };
  out.writeln("randomised runs (16 seeds): bias of the mean and standard deviation of one run");
  for (std::int64_t paths : {1024, 4096, 16384}) {
    prc::MonteCarloOptions pseudo;
    pseudo.paths = paths;
    pseudo.steps = 16;
    prc::MonteCarloOptions sobol = pseudo;
    sobol.rand_method = vol::RandMethod::Sobol;
    sobol.randomized_quasi = true;
    prc::MonteCarloOptions scrambled = sobol;
    scrambled.sobol_scrambling = 2;
    const auto a = spread(pseudo);
    const auto b = spread(sobol);
    const auto c = spread(scrambled);
    out.writeln("  paths", paths, " pseudo", a.first, a.second, " | randomised Sobol", b.first, b.second, " | scrambling 2", c.first, c.second);
  }
}
```

Output

```text
COS reference 8.5423
deterministic sequences, 16384 paths, 16 steps: error against the reference
   Pseudo  0.0359
   Sobol   0.0080
   Halton  -0.1594
   Torus   -0.0518
randomised runs (16 seeds): bias of the mean and standard deviation of one run
  paths 1024  pseudo 0.0155 0.2831  | randomised Sobol -0.0349 0.3474  | scrambling 2 -0.0377 0.3708
  paths 4096  pseudo 0.0036 0.1840  | randomised Sobol 0.0078 0.1372  | scrambling 2 -0.0381 0.2367
  paths 16384  pseudo 0.0265 0.0986  | randomised Sobol 0.0659 0.1330  | scrambling 2 0.0301 0.0982
```

Among the deterministic sets at 16384 paths and 16 steps (32 dimensions), Sobol is the most accurate, with an error of 0.008, against 0.036 for the pseudo-random run, 0.052 for Torus and 0.159 for Halton, whose later dimensions use large prime bases and are poorly distributed in 32 dimensions. A deterministic set has no error bar. The randomised runs show that, for this problem, randomisation gives no consistent gain over pseudo-random numbers: the standard deviation of one run is 0.347, 0.137 and 0.133 for randomised Sobol at 1024, 4096 and 16384 paths against 0.283, 0.184 and 0.099 for pseudo-random numbers, and a scrambling of 2 gives 0.098 at 16384 paths. A path-dependent simulation with many steps is a high-dimensional integral, where quasi-random sequences lose most of their advantage.

## Seeds, offsets and antithetic pairs

For a pseudo-random run the `seed` selects the stream. For a deterministic quasi-random run the sequence is fixed and `seed` has no effect; `offset` moves the starting point along it, so different offsets give different, equally good point sets. A randomised sequence uses the `seed` for the randomisation. The program shows each, and that antithetic pairing can be combined with a quasi-random sequence.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace prc = pluss::subsp::math::finance::pricing;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const vol::HestonParams params;
  const double reference = prc::heston_price_cos(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params);
  auto price = [&](prc::MonteCarloOptions options) { return prc::heston_price_mc(prc::Call, 100.0, 100.0, 0.03, 0.01, 1.0, params, options); };

  prc::MonteCarloOptions sobol;
  sobol.paths = 8192;
  sobol.steps = 16;
  sobol.rand_method = vol::RandMethod::Sobol;
  out.writeln("deterministic Sobol: seed 1", price([&] { auto o = sobol; o.seed = 1; return o; }()), " seed 2", price([&] { auto o = sobol; o.seed = 2; return o; }()), "(no effect)");
  out.writeln("                     offset 0", price(sobol), " offset 8192", price([&] { auto o = sobol; o.offset = 8192; return o; }()), " offset 16384", price([&] { auto o = sobol; o.offset = 16384; return o; }()));
  out.writeln("randomised:          seed 1", price([&] { auto o = sobol; o.randomized_quasi = true; o.seed = 1; return o; }()), " seed 2", price([&] { auto o = sobol; o.randomized_quasi = true; o.seed = 2; return o; }()));
  out.writeln("Sobol + antithetic   deterministic", price([&] { auto o = sobol; o.antithetic = true; return o; }()), " randomised", price([&] { auto o = sobol; o.antithetic = true; o.randomized_quasi = true; return o; }()));
  out.writeln("reference", reference);
}
```

Output

```text
deterministic Sobol: seed 1 8.48051  seed 2 8.48051 (no effect)
                     offset 0 8.48051  offset 8192 8.61998  offset 16384 8.59904
randomised:          seed 1 8.49669  seed 2 8.72938
Sobol + antithetic   deterministic 8.45686  randomised 8.55551
reference 8.54226
```

A deterministic Sobol run does not depend on the seed (8.48051 for both) and depends on the offset (8.4805, 8.6200 and 8.5990 for three starting points, each a different point set of the same quality). A randomised run depends on the seed (8.4967 and 8.7294). Antithetic pairing combines with both (8.4569 and 8.5555).
