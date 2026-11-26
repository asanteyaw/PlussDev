# Host and parasitoid generations

[Back to Biology 37: Host-Parasitoid Maps](../bio_37_parasitoids.md)

A parasitoid lays its eggs in a host and the host dies, and both species breed once a year. You want to know whether the two can coexist, what density each settles at, and what makes the interaction stable. This page iterates the classic Nicholson-Bailey model, shows that its coexistence point is always unstable, shows how clumped attacks stabilise it, and links patches by dispersal.

## How it works

The code is in `pluss::ma::natsci::biology::population::parasitoid` (`parasitoid::` below). The state is a pair (hosts {math}`H`, parasitoids {math}`P`) on the last axis. Hosts reproduce with rate {math}`\lambda` and escape attack with probability {math}`f(P)`; each host that is found yields {math}`c` parasitoids:

```{math}
\begin{aligned}
&H_{t+1} = \lambda H_t f(P_t), \\
&P_{t+1} = c H_t \bigl(1 - f(P_t)\bigr)
\end{aligned}
```

With attacks spread at random, {math}`f = e^{-aP}`, which is `parasitoid::nicholson_bailey(state, lambda, a, c)`. Clumped attacks, with the number of attacks per host negative binomial with clumping parameter {math}`k`, give {math}`f = (1 + aP/k)^{-k}` in `parasitoid::negative_binomial(state, lambda, a, c, k)`, and `parasitoid::beddington` adds host density dependence. The coexistence point is returned by `parasitoid::nicholson_bailey_equilibrium(lambda, a, c, like)` and `parasitoid::negative_binomial_equilibrium(lambda, a, c, k, like)` (the last argument only supplies the shape and type of the result), `parasitoid::jacobian(map, state)` differentiates a map at a state and `parasitoid::spectral_radius(J)` returns the largest modulus of the eigenvalues of a {math}`2\times2` Jacobian. The point is stable when that radius is below one. For patches on a ring, `parasitoid::ring_step(local, n, d)` applies a local map in every patch and then lets a fraction {math}`d` of each patch's output disperse equally to its two neighbours, and `parasitoid::synchrony(n)` is the standard deviation of the densities across patches.

## Random search is unstable

The program computes the coexistence point for {math}`\lambda = 2`, {math}`a = 0.05`, {math}`c = 1`, the spectral radius of its Jacobian, and follows a population started `1%` above it for `60` generations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace parasitoid = biology::population::parasitoid;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Nicholson-Bailey: hosts reproduce at rate lambda, parasitoids search with area a and lay c eggs per host found
  const double lambda = 2.0, a = 0.05, c = 1.0;
  const auto eq = parasitoid::nicholson_bailey_equilibrium(lambda, a, c, vec({0.0}));
  out.writeln("coexistence point (hosts, parasitoids) = ", flat(eq));
  const auto f = [&](const pluss::Series &s) { return parasitoid::nicholson_bailey(s, lambda, a, c); };
  const auto J = parasitoid::jacobian(f, eq);
  out.writeln("spectral radius of the Jacobian = ", flat(parasitoid::spectral_radius(J))[0]);
  // start 1% away from the point and follow the host density
  auto s = eq.mul(vec({1.01, 1.0}));
  for (int t = 0; t <= 60; ++t) {
    if (t % 10 == 0) out.writeln("generation ", t, ": hosts ", flat(s)[0], ", parasitoids ", flat(s)[1]);
    s = f(s);
  }
}
```

Output

```text
coexistence point (hosts, parasitoids) = [27.7259, 13.8629]
spectral radius of the Jacobian = 1.1774
generation 0: hosts 28.0031, parasitoids 13.8629
generation 10: hosts 28.2307, parasitoids 14.7326
generation 20: hosts 21.5882, parasitoids 15.0843
generation 30: hosts 10.8698, parasitoids 2.0160
generation 40: hosts 0.0144, parasitoids 0.0000
generation 50: hosts 14.7600, parasitoids 0.0000
generation 60: hosts 15114.2522, parasitoids 0.0000
```

The point is `27.7259` hosts and `13.8629` parasitoids, and the spectral radius is `1.1774 > 1`. A one per cent disturbance therefore grows, with oscillations of increasing amplitude: after `30` generations the hosts have fallen to `10.8698`, after `40` they are at `0.0144` and the parasitoids are extinct, and with no parasitoid left the hosts grow by a factor of {math}`\lambda = 2` each generation (`14.7600` at generation `50`, `15114.2522` at `60`).

## Clumped attacks stabilise the interaction

Attacks are rarely spread evenly over hosts. The program computes the equilibrium and its stability for six values of the clumping parameter {math}`k`, from nearly random (`100`) to strongly clumped (`0.2`).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace parasitoid = biology::population::parasitoid;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // aggregation of attacks stabilises: negative binomial escape (1 + aP/k)^-k, with small k meaning strongly clumped attacks
  const double lambda = 2.0, a = 0.05, c = 1.0;
  for (double k : {100.0, 2.0, 1.2, 0.8, 0.5, 0.2}) {
    const auto eq = parasitoid::negative_binomial_equilibrium(lambda, a, c, k, vec({0.0}));
    const auto f = [&](const pluss::Series &s) { return parasitoid::negative_binomial(s, lambda, a, c, k); };
    const auto rho = parasitoid::spectral_radius(parasitoid::jacobian(f, eq));
    out.writeln("k = ", k, ": equilibrium ", flat(eq), ", spectral radius ", flat(rho)[0], flat(rho)[0] < 1.0 ? "  stable" : "  unstable");
  }
}
```

Output

```text
k = 100.0000: equilibrium [27.8222, 13.9111], spectral radius 1.1754  unstable
k = 2.0000: equilibrium [33.1371, 16.5685], spectral radius 1.0824  unstable
k = 1.2000: equilibrium [37.5263, 18.7631], spectral radius 1.0262  unstable
k = 0.8000: equilibrium [44.1093, 22.0546], spectral radius 0.9630  stable
k = 0.5000: equilibrium [60.0000, 30.0000], spectral radius 0.8660  stable
k = 0.2000: equilibrium [248.0000, 124.0000], spectral radius 0.6225  stable
```

Large {math}`k` reproduces the random-search model (radius `1.1754` at {math}`k = 100`). The radius falls as attacks get more clumped and crosses one at {math}`k = 1`: it is `1.0262` at {math}`k = 1.2` (unstable) and `0.9630` at {math}`k = 0.8` (stable), and `0.6225` at {math}`k = 0.2`. Clumping also raises the equilibrium, from `27.8222` hosts to `248.0000`, because hosts in a heavily attacked clump cost the parasitoids little, so many other hosts escape.

## Patches linked by dispersal

Dispersal can also change the dynamics of a population in space. The program runs eight patches on a ring, each with a chaotic Ricker map, for `600` generations at weak and at strong dispersal.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace maps = biology::population::maps;
namespace parasitoid = biology::population::parasitoid;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // eight patches on a ring, each running a chaotic Ricker map (r = 2.8); a fraction d of each patch's offspring disperses equally to its two neighbours
  const auto local = [](const pluss::Series &n) { return maps::ricker(n, 2.8); };
  const auto start = vec({0.2, 0.5, 0.9, 1.4, 0.3, 0.7, 1.1, 0.4});
  for (double d : {0.02, 0.5}) {
    auto n = start;
    for (int t = 0; t < 600; ++t) n = parasitoid::ring_step(local, n, d);
    out.writeln("dispersal ", d, ": spread across patches ", flat(parasitoid::synchrony(n))[0], ", patch densities ", flat(n));
  }
}
```

Output

```text
dispersal 0.0200: spread across patches 0.4607, patch densities [0.1721, 0.6914, 0.4775, 1.3818, 0.5219, 0.1870, 1.4495, 0.4620]
dispersal 0.5000: spread across patches 0.1328, patch densities [0.2812, 0.4393, 0.4477, 0.2955, 0.1551, 0.1028, 0.1012, 0.1477]
```

With a dispersal fraction of `0.02` the patches stay different: the standard deviation across patches is `0.4607` and their densities range from `0.1721` to `1.4495`. With `0.5` the patches are pulled together and the spread falls to `0.1328`, so dispersal synchronises neighbouring populations, which makes them go extinct together more easily.

## Decisions

- Use `nicholson_bailey` to see the baseline and `negative_binomial` (or `beddington`) when you want coexistence; the random-search model has no stable coexistence for any parameters, so fitting it to a persistent system is a modelling error.
- Test stability with `jacobian` and `spectral_radius`, and confirm by iterating from a nearby state, as the first example does.
- Give `ring_step` the single-patch map, so that any map from the `maps` family or your own can be placed in space.

## Pitfalls

The state is (hosts, parasitoids), in that order, on the last axis. The program enters the equilibrium once correctly and once with the two numbers swapped, then tries a state with three components.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace parasitoid = biology::population::parasitoid;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the state is a pair (hosts, parasitoids) on the LAST axis: reversing the order swaps the roles
  const double lambda = 2.0, a = 0.05, c = 1.0;
  const auto eq = parasitoid::nicholson_bailey_equilibrium(lambda, a, c, vec({0.0}));
  const auto good = parasitoid::nicholson_bailey(eq, lambda, a, c);
  const auto swapped = parasitoid::nicholson_bailey(vec({flat(eq)[1], flat(eq)[0]}), lambda, a, c);
  out.writeln("equilibrium (hosts, parasitoids) = ", flat(eq), "; one step later ", flat(good));
  out.writeln("same numbers entered as (parasitoids, hosts); one step later ", flat(swapped));
  // a state with the wrong number of components is not a pair at all
  try {
    parasitoid::nicholson_bailey(vec({1.0, 2.0, 3.0}), lambda, a, c);
    out.writeln("three components accepted, result ", flat(parasitoid::nicholson_bailey(vec({1.0, 2.0, 3.0}), lambda, a, c)));
  } catch (const pluss::Error &e) { out.writeln("error: ", e.message()); }
}
```

Output

```text
equilibrium (hosts, parasitoids) = [27.7259, 13.8629]; one step later [27.7259, 13.8629]
same numbers entered as (parasitoids, hosts); one step later [6.9315, 10.3972]
error: natsci: parasitoid::nicholson_bailey: state (hosts, parasitoids) must have 2 species on its last axis
```

At the equilibrium one step returns the same pair, `[27.7259, 13.8629]`. The swapped pair is a valid state, so nothing complains, and one step gives `[6.9315, 10.3972]`, a different population. A state that does not have two components is rejected with an error. Keep the order the same everywhere you build states.

## Where next

[Biology 36: Discrete-Time Maps and Chaos](../bio_36_maps.md) analyses the one-species maps used for the local dynamics here, [Biology 2: Interacting Species](../bio_02_interactions.md) treats predators and prey in continuous time, and [Biology 38: Metapopulations and Harvest](../bio_38_metapopulations.md) asks when patches stay occupied.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
