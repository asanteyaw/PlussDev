# Random communities

[Back to Biology 29: Community Stability and Assembly](../bio_29_communities.md)

You want to know whether a community with many species and a haphazard web of interactions can persist: does diversity help or hurt stability, and what difference does the kind of interaction make? This page draws many random communities at once, measures how close each is to losing stability, and compares the result with the large-community prediction, for random, predator-prey and competitive interactions.

## How it works

Close to an equilibrium, the departures of $S$ species obey $\dot x = Mx$ with a community matrix $M$ that has $-d$ on the diagonal (every species regulates itself at rate $d$) and off-diagonal entries that are zero with probability $1 - C$ (the connectance $C$ is the fraction of pairs that interact) and otherwise random with standard deviation $\sigma$. The pair correlation $\tau = E[M_{ij}M_{ji}]/\sigma^2$ says whether partners affect each other alike ($\tau>0$, competition or mutualism), oppositely ($\tau<0$, predator and prey) or independently ($\tau = 0$). The community is stable when every eigenvalue of $M$ has a negative real part, that is when the *spectral abscissa* $\max\mathrm{Re}\,\lambda$ is negative. For many species the eigenvalues fill an ellipse around $-d$ and the edge of the stability region is

```{math}
\max\mathrm{Re}\,\lambda \approx \sigma\sqrt{SC}\,(1+\tau) - d
```

so that with $\tau = 0$ the community is stable when $\sigma\sqrt{SC} < d$: the more species and the stronger the interactions, the less stable. `community::random_community(species, lanes, sigma, connectance, tau, d, seed)` draws `lanes` independent matrices as one Series of shape `[lanes, species, species]`; `community::spectral_abscissa(M)` returns the abscissa of every matrix in the batch; `community::predicted_abscissa(species, sigma, connectance, tau, d)` is the formula. A seed of `0` draws a fresh random community each time; any other seed makes the draw repeatable.

## Drawing a community

The program draws one community of four species, prints it row by row, checks the diagonal, and shows that the seed controls the draw.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a random community of 4 species: -1 on the diagonal (each species regulates itself) and random interactions of standard deviation 0.5
  const auto m = community::random_community(4, 1, 0.5, 1.0, 0.0, 1.0, 5);
  out.writeln("shape of the batch  ", std::vector<double>{static_cast<double>(m.size(0)), static_cast<double>(m.size(1)), static_cast<double>(m.size(2))});
  const auto a = m.select(0, 0);
  for (std::int64_t i = 0; i < 4; ++i) out.writeln("row ", std::vector<double>{static_cast<double>(i)}, " ", flat(a.select(0, i)));
  out.writeln("diagonal            ", flat(a.diagonal()));
  // a different seed gives a different community, the same seed the same one
  const auto same = community::random_community(4, 1, 0.5, 1.0, 0.0, 1.0, 5), other = community::random_community(4, 1, 0.5, 1.0, 0.0, 1.0, 6);
  out.writeln("same seed, largest difference       ", std::vector<double>{same.sub(m).abs().max().item<double>()});
  out.writeln("other seed, largest difference      ", std::vector<double>{other.sub(m).abs().max().item<double>()});
}
```

Output

```text
shape of the batch  [1.0000, 4.0000, 4.0000]
row [0.0000] [-1.0000, 0.1285, 0.9451, 0.7382]
row [1.0000] [0.0022, -1.0000, 0.2660, 0.8676]
row [2.0000] [-0.8564, 0.5201, -1.0000, 0.0123]
row [3.0000] [0.1428, -0.1496, -0.2580, -1.0000]
diagonal            [-1.0000, -1.0000, -1.0000, -1.0000]
same seed, largest difference       [0.0000]
other seed, largest difference      [1.1433]
```

The result has shape `[1, 4, 4]`: one community of four species. The diagonal is `-1.0000` in all four places, the self-regulation $-d$, and the off-diagonal entries are the random interaction strengths, positive for a species that helps another and negative for one that harms it. Drawing again with the same seed reproduces the community exactly (difference `0.0000`), and with another seed changes entries by as much as `1.1433`.

## Stability against interaction strength

The program draws `400` communities of `60` fully connected species at five interaction strengths, measures the spectral abscissa of all of them in one call, and prints the prediction, the average edge and the fraction of stable communities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // 400 communities of 60 species at once, each with interactions of standard deviation sigma and every pair connected (connectance 1).
  // d = 1: the community is stable when the real part of every eigenvalue is negative. May's rule: stable when sigma sqrt(S C) < d
  const std::int64_t species = 60, lanes = 400;
  for (double sigma : {0.05, 0.10, 0.13, 0.16, 0.25}) {
    const auto m = community::random_community(species, lanes, sigma, 1.0, 0.0, 1.0, 21);
    const auto abscissa = community::spectral_abscissa(m);
    const double predicted = community::predicted_abscissa(species, sigma, 1.0, 0.0, 1.0);
    out.writeln("sigma sqrt(S C) = ", std::vector<double>{sigma * std::sqrt(static_cast<double>(species))},
                "  predicted edge ", std::vector<double>{predicted},
                "  mean edge ", std::vector<double>{abscissa.mean().item<double>()},
                "  stable fraction ", std::vector<double>{abscissa.lt(0.0).to(pluss::ScalarType::Float64).mean().item<double>()});
  }
}
```

Output

```text
sigma sqrt(S C) = [0.3873]  predicted edge [-0.6127]  mean edge [-0.6359]  stable fraction [1.0000]
sigma sqrt(S C) = [0.7746]  predicted edge [-0.2254]  mean edge [-0.2717]  stable fraction [1.0000]
sigma sqrt(S C) = [1.0070]  predicted edge [0.0070]  mean edge [-0.0532]  stable fraction [0.7675]
sigma sqrt(S C) = [1.2394]  predicted edge [0.2394]  mean edge [0.1653]  stable fraction [0.0475]
sigma sqrt(S C) = [1.9365]  predicted edge [0.9365]  mean edge [0.8207]  stable fraction [0.0000]
```

The measured edge follows the formula: it climbs from `-0.6359` to `0.8207` as $\sigma\sqrt{SC}$ rises from `0.39` to `1.94`, always a little below the prediction, `-0.6127` to `0.9365`. The fraction of stable communities falls from `1.0000` through `0.7675` and `0.0475` to `0.0000`; it drops sharply around $\sigma\sqrt{SC} = d = 1$, May's threshold, and at `1.0070`, where the prediction is just barely unstable (`0.0070`), still `77%` of the finite communities are stable.

## Predator-prey, random and competitive interactions

The program holds the interaction strength at $\sigma\sqrt{SC} = 0.8\,d$ and changes only the pair correlation $\tau$, from `-0.9` through `-2/pi`, `0` and `2/pi` to `0.9`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the pair correlation tau sets the kind of interaction: tau = 0 random, tau < 0 predator-prey (what helps one hurts the other), tau > 0 competition or mutualism.
  // For the same strength of interaction (sigma sqrt(S C) = 0.8 d) the stability differs
  const std::int64_t species = 60, lanes = 400;
  const double sigma = 0.8 / std::sqrt(60.0);
  for (double tau : {-0.9, -2.0 / M_PI, 0.0, 2.0 / M_PI, 0.9}) {
    const auto m = community::random_community(species, lanes, sigma, 1.0, tau, 1.0, 33);
    const auto abscissa = community::spectral_abscissa(m);
    out.writeln("tau = ", std::vector<double>{tau}, "  predicted edge ", std::vector<double>{community::predicted_abscissa(species, sigma, 1.0, tau, 1.0)},
                "  mean edge ", std::vector<double>{abscissa.mean().item<double>()}, "  stable fraction ", std::vector<double>{abscissa.lt(0.0).to(pluss::ScalarType::Float64).mean().item<double>()});
  }
}
```

Output

```text
tau = [-0.9000]  predicted edge [-0.9200]  mean edge [-0.9109]  stable fraction [1.0000]
tau = [-0.6366]  predicted edge [-0.7093]  mean edge [-0.7143]  stable fraction [1.0000]
tau = [0.0000]  predicted edge [-0.2000]  mean edge [-0.2542]  stable fraction [1.0000]
tau = [0.6366]  predicted edge [0.3093]  mean edge [0.2233]  stable fraction [0.0000]
tau = [0.9000]  predicted edge [0.5200]  mean edge [0.4270]  stable fraction [0.0000]
```

At the same strength the kind of interaction decides the fate of the community. For $\tau = -0.9$ and $\tau = -2/\pi = -0.6366$ (predator-prey) the edge is far below zero, `-0.9109` and `-0.7143`, and all communities are stable. Random interactions ($\tau = 0$) have edge `-0.2542` and are stable too. Competition or mutualism ($\tau = 2/\pi$ and `0.9`) pushes the edge to `0.2233` and `0.4270` and not one community in `400` is stable. Exploitative interactions stabilise a community and symmetric ones destabilise it.

## Decisions

- Draw at least a few hundred lanes: the stable fraction is an estimate and its resolution is one over the number of lanes.
- Use the formula `predicted_abscissa` to choose the range of $\sigma$ to scan, and the simulated communities to see the finite-size behaviour that the formula does not give.
- Use `d` to set the units: only the ratio of $\sigma\sqrt{SC}$ to $d$ matters, so you can set $d = 1$.
- Give a fixed `seed` in documentation or tests, and `0` when you want a fresh sample.

## Pitfalls

Invalid arguments are rejected with a message that names the parameter. The program asks for one species and for a connectance above one, and then compares the formula with simulations for `5`, `20` and `80` species at the same relative strength.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // out of range arguments are rejected
  try {
    community::random_community(1, 10, 0.5, 1.0, 0.0, 1.0);
  } catch (const pluss::Error &e) {
    out.writeln("one species: ", e.message());
  }
  try {
    community::random_community(10, 10, 0.5, 1.2, 0.0, 1.0);
  } catch (const pluss::Error &e) {
    out.writeln("connectance above one: ", e.message());
  }
  // a small community does not follow the large-community formula closely: the prediction is for many species
  for (std::int64_t species : {5, 20, 80}) {
    const auto m = community::random_community(species, 300, 0.8 / std::sqrt(static_cast<double>(species)), 1.0, 0.0, 1.0, 9);
    out.writeln("S = ", std::vector<double>{static_cast<double>(species)}, "  predicted ", std::vector<double>{community::predicted_abscissa(species, 0.8 / std::sqrt(static_cast<double>(species)), 1.0, 0.0, 1.0)},
                "  mean ", std::vector<double>{community::spectral_abscissa(m).mean().item<double>()},
                "  spread ", std::vector<double>{community::spectral_abscissa(m).std().item<double>()});
  }
}
```

Output

```text
one species: natsci: random_community: need at least two species
connectance above one: natsci: random_community: connectance must lie in (0, 1]
S = [5.0000]  predicted [-0.2000]  mean [-0.5698]  spread [0.2045]
S = [20.0000]  predicted [-0.2000]  mean [-0.3091]  spread [0.1159]
S = [80.0000]  predicted [-0.2000]  mean [-0.2328]  spread [0.0560]
```

Each rejection names its parameter. The formula is for many species. At the same $\sigma\sqrt{S}=0.8$ it predicts an edge of `-0.2000` whatever $S$ is, but `5` species give an average edge of `-0.5698`, `20` give `-0.3091` and `80` give `-0.2328`. The spread among communities, `0.2045`, `0.1159`, `0.0560`, shrinks as the community grows, so a large community is predictable and a small one is a matter of chance. Do not apply the formula to a handful of species.

## Where next

Continue with [Assembly and chemostats](assembly.md) for communities built from given growth rates and interaction matrices, or go to [Biology 2: Interacting Species](../bio_02_interactions.md) for a three-species community studied in detail.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assembly.html">
    <span class="label">Next</span>
    <span class="title">Assembly and chemostats</span>
  </a>
</div>
<!-- apiref:nav:end -->
