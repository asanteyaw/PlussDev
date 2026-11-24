# Flux sampling

[Back to Biology 32: The Flux Space](../bio_32_flux_space.md)

Flux balance gives one optimal flux and flux variability gives the range of each flux, but neither says how the fluxes are spread inside the region the network allows. You want typical fluxes, their correlations and their distribution. This page samples the polytope of steady-state fluxes by hit-and-run, from a chosen starting point, with several chains at once, and compares the samples with the exact ranges.

## How it works

The allowed fluxes form the polytope $\{v : Sv = 0,\ l\le v\le u\}$. Hit-and-run starts at an interior point, picks a random direction in the nullspace of $S$, finds the interval along that direction that stays inside the bounds by ratio tests, and jumps to a uniformly random point of that interval. Repeated, it generates samples that are asymptotically uniform over the polytope. `metabolism::sample_fluxes(net, chains, samples, thinning, seed, options)` runs `chains` independent chains together, keeps every `thinning`-th step, and returns a `Sample` with `fluxes` of shape `[chains, samples, reactions]`, the Chebyshev centre (the centre of the largest ball inside the polytope, which is the start) and its radius, the list of reactions that the network holds at a single value (`fixed_reactions`), and the number of unbounded sides that were truncated at `bound_cap` (default `1000`). A reaction pinned by `l = u` or by the network is held at its value and the sampling goes on in the remaining directions; a polytope with no interior at all is refused. A fixed `seed` makes the samples repeatable.

## Sampling the bypass network

The program takes four chains of `500` samples with a thinning of `10`, and prints the shape of the result, the Chebyshev centre and radius, and the mean, smallest and largest sampled flux of every reaction, and checks that every sample is a steady state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = bypass();
  // sample the flux space: 4 chains of 500 samples each, 10 steps apart, from a fixed seed
  const auto sample = metabolism::sample_fluxes(net, 4, 500, 10, 3);
  out.writeln("shape [chains, samples, reactions] ", std::vector<double>{static_cast<double>(sample.fluxes.size(0)), static_cast<double>(sample.fluxes.size(1)), static_cast<double>(sample.fluxes.size(2))});
  out.writeln("Chebyshev centre (the start) ", flat(sample.chebyshev_centre));
  out.writeln("Chebyshev radius              ", std::vector<double>{sample.chebyshev_radius});
  out.writeln("reactions held at one value   ", std::vector<double>{static_cast<double>(sample.fixed_reactions.size())});
  out.writeln("bounds that were truncated    ", std::vector<double>{static_cast<double>(sample.truncated_bounds)});
  const auto flux = sample.fluxes.reshape({2000, 5});
  out.writeln("mean flux of each reaction    ", flat(flux.mean(0)));
  out.writeln("smallest sampled flux         ", flat(flux.amin(0, false)));
  out.writeln("largest sampled flux          ", flat(flux.amax(0, false)));
  out.writeln("largest |S v| over all samples ", std::vector<double>{net.stoichiometry.matmul(flux.transpose(0, 1)).abs().max().item<double>()});
}
```

Output

```text
shape [chains, samples, reactions] [4.0000, 500.0000, 5.0000]
Chebyshev centre (the start) [7.0000, 3.0000, 4.0000, 4.0000, 7.0000]
Chebyshev radius              [3.0000]
reactions held at one value   [0.0000]
bounds that were truncated    [0.0000]
mean flux of each reaction    [6.2316, 2.7494, 3.4822, 3.4822, 6.2316]
smallest sampled flux         [0.2473, 0.0038, 0.0004, 0.0004, 0.2473]
largest sampled flux          [9.9927, 5.9903, 7.9945, 7.9945, 9.9927]
largest |S v| over all samples [0.0000]
```

The samples have shape `[4, 500, 5]`. The Chebyshev centre `(7, 3, 4, 4, 7)` is the start and its radius is `3.0000`. No reaction is held fixed and no bound was truncated. The mean of the uptake is `6.2316`, so the sampler spends most of its time at moderate to high uptake, and the sampled fluxes cover the ranges `0.25` to `9.99` for uptake and `0.0038` to `5.99` for $A\to B$, essentially the full allowed ranges `[0, 10]` and `[0, 6]`. Every sample is a steady state: the largest entry of $S v$ over the `2000` samples is `0.0000`.

## Sampling at the optimum

The program pins the uptake and the biomass at `10`, the optimum, by setting their lower bounds, and samples the alternatives that remain.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = bypass();
  // the same network at the biomass optimum: pin the biomass flux at its maximum and sample the alternatives. Uptake = 10 forces everything to balance
  auto fixed = net;
  fixed.lower = vec({10.0, 0.0, 0.0, 0.0, 10.0});
  const auto sample = metabolism::sample_fluxes(fixed, 4, 500, 10, 3);
  const auto flux = sample.fluxes.reshape({2000, 5});
  out.writeln("mean fluxes          ", flat(flux.mean(0)));
  out.writeln("smallest fluxes      ", flat(flux.amin(0, false)));
  out.writeln("largest fluxes       ", flat(flux.amax(0, false)));
  out.writeln("reactions held fixed ", std::vector<double>{static_cast<double>(sample.fixed_reactions.size())});
  // the flux through A->B can be anywhere between 2 and 6 (the range of flux variability at the optimum); its mean shows how the sampler spreads
  const auto fba = metabolism::flux_variability(fixed, vec({0.0, 0.0, 0.0, 0.0, 1.0}));
  out.writeln("variability range of A->B ", std::vector<double>{flat(fba.minimum)[1], flat(fba.maximum)[1]});
}
```

Output

```text
mean fluxes          [10.0000, 3.9666, 6.0334, 6.0334, 10.0000]
smallest fluxes      [10.0000, 2.0043, 4.0009, 4.0009, 10.0000]
largest fluxes       [10.0000, 5.9991, 7.9957, 7.9957, 10.0000]
reactions held fixed [2.0000]
variability range of A->B [2.0000, 6.0000]
```

The uptake and the biomass are held at `10` exactly and reported as fixed (`2` fixed reactions). The detour fluxes vary freely between the bounds that flux variability gives, `2` to `6` for $A\to B$ and `4` to `8` for the other two, and the sampled minima `2.0043` and maximum `5.9991` for $A\to B$ agree. The means `3.9666` and `6.0334` show the sampler centred in the range, `4` and `6`, and the two always sum to the uptake `10`.

## The error cases

The program asks for samples of a network whose bounds cannot be met and for zero chains.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a network whose bounds cannot be met has nothing to sample: the biomass flux is required to be at least 20, but uptake is limited to 10
  metabolism::Network blocked = bypass();
  blocked.lower = vec({0.0, 0.0, 0.0, 0.0, 20.0});
  try {
    metabolism::sample_fluxes(blocked, 2, 50, 5, 1);
  } catch (const pluss::Error &e) {
    out.writeln("infeasible network: ", e.message());
  }
  try {
    metabolism::sample_fluxes(bypass(), 0, 50, 5, 1);
  } catch (const pluss::Error &e) {
    out.writeln("zero chains: ", e.message());
  }
}
```

Output

```text
infeasible network: natsci: flux_variability: the reference optimum was not found
zero chains: natsci: sample_fluxes: bounds must be [r]; chains, samples, thinning positive
```

An infeasible network is refused, because the sampler first finds the range of each flux and there is none. A request for zero chains is refused. Both messages name the function.

## Decisions

- Use many short chains rather than one long one: the chains advance together as lanes and give independent starting points for convergence checks.
- Fix the objective at the optimum, or at a fraction of it, by raising its lower bound, to sample the near-optimal flux space.
- Increase `thinning` when neighbouring samples look too similar, and increase `samples` for smoother distributions.
- Treat truncated bounds with care: if many sides are truncated, the distribution depends on the arbitrary cap.

## Pitfalls

A reaction with no upper bound enlarges the polytope, and the sampler has to cut it off. The program gives $C\to B$ an infinite upper bound and samples it, and compares with the bound of `8`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the bypass network with the C -> B reaction given no upper bound
  auto net = bypass();
  net.upper = vec({10.0, 6.0, 100.0, std::numeric_limits<double>::infinity(), 100.0});
  const auto sample = metabolism::sample_fluxes(net, 4, 500, 10, 3);
  out.writeln("truncated bounds ", std::vector<double>{static_cast<double>(sample.truncated_bounds)});
  const auto flux = sample.fluxes.reshape({2000, 5});
  out.writeln("mean flux of C -> B        ", flat(flux.select(1, 3).mean()));
  out.writeln("largest sampled flux of C -> B ", std::vector<double>{flux.select(1, 3).amax(0, false).item<double>()});
  // with a bound of 8 the same reaction is limited by the network itself
  const auto bounded = metabolism::sample_fluxes(bypass(), 4, 500, 10, 3);
  out.writeln("with the bound 8: mean     ", flat(bounded.fluxes.reshape({2000, 5}).select(1, 3).mean()));
}
```

Output

```text
truncated bounds [1.0000]
mean flux of C -> B        [3.7256]
largest sampled flux of C -> B [9.8003]
with the bound 8: mean     [3.4822]
```

The sampler reports `1` truncated bound: it replaced the infinite side by the cap of `1000`. Here the uptake limit of `10` bounds the flux anyway, but the polytope is larger than with the bound of `8` (the largest sampled flux is `9.8003`, above `8`), and the mean flux of the reaction rises from `3.4822` to `3.7256`. Read `truncated_bounds` after every sampling run: a non-zero count means the samples depend on a cap you chose by default, and in a large network the effect can be dominant. Give every reaction a physiological bound.

## Where next

Go back to [Elementary modes](elementary_modes.md) for the pathways, and to [Biology 33: Loopless Flux Analysis](../bio_33_loopless.md) for removing the thermodynamically infeasible cycles that unbounded sampling would otherwise include.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="elementary_modes.html">
    <span class="label">Previous</span>
    <span class="title">Elementary modes</span>
  </a>
</div>
<!-- apiref:nav:end -->
